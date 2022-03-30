#include "my_topology.h"
#include "mig_manager.h"
#include "orchestrator.h"
#include <iostream>
#include <sstream>
#include <algorithm>    
#include <stack> 

static class MyTopologyClass : public TclClass {
public:
	MyTopologyClass() : TclClass("MyTopology") {}
	TclObject* create(int, const char*const*) {
		return (new MyTopology());
	}
} my_class_topology;

MyTopology* MyTopology::instance_ = nullptr;

MyTopology::MyTopology(){
    bind_bool("verbose_", &verbose); // TODO: fix this. Doesn't work. 
    verbose = false; 

    mig_root = nullptr; 
    mig_manager_ = new MigrationManager(); 
    instance_ = this; 
}

MyTopology::~MyTopology(){
	delete mig_manager_; 
}

MigrationManager& MyTopology::mig_manager(){
    return *mig_manager_; 
};

void MyTopology::tcl_command(const std::list<std::string> & myArguments) {
    std::stringstream command;
    for (auto const& i : myArguments) {
        command << i << " ";
    }
    if(verbose){
        std::cout << command.str() << std::endl;
    }
    Tcl::instance().evalf(command.str().c_str());
}

int MyTopology::command(int argc, const char*const* argv){

    Tcl& tcl = Tcl::instance();

    if (strcmp(argv[1], "find_node") == 0) {

        Node* root = (Node*)TclObject::lookup(argv[2]);
        std::vector<int> branch_ids;
        for(int i = 3; i < argc; i++){
            branch_ids.push_back(atoi(argv[i]) - 1);
        } 
        Node* node = find_node(root, branch_ids);
        tcl.resultf("%s", data[node].pointer.c_str());
        return TCL_OK;

    } else if (strcmp(argv[1], "make_tree") == 0) {
        Node* root = (Node*)TclObject::lookup(argv[2]);
        
        std::vector<int> branching_ds;
        for(int i = 3; i < argc; i++){
            branching_ds.push_back(atoi(argv[i]));
        } 
        make_tree(root, branching_ds);
        
        return TCL_OK;  
    } else if (argc == 2) {
        if (strcmp(argv[1], "start_migration") == 0){
            auto& orch = BaseOrchestrator::instance();
            orch.setup_node_types();
            orch.start_migration();

            return TCL_OK; 
        } else if (strcmp(argv[1], "print_nodes") == 0) {
            print_nodes();
            return TCL_OK; 
        } else if (strcmp(argv[1], "print_graph") == 0) {
            print_graph();
            return TCL_OK; 
        } else if (strcmp(argv[1], "setup_apps") == 0) {
            setup_apps();
            return TCL_OK; 
        } else if (strcmp(argv[1], "make_node") == 0) {
            Node* n = make_node(); 
            tcl.resultf("%s", data[n].pointer.c_str());
            return TCL_OK;
        } 
    } else if (argc == 3) {
        if (strcmp(argv[1], "deactivate_tunnel") == 0) { 
			int uid = atoi(argv[2]);
            mig_manager().deactivate_tunnel(uid); 
			return(TCL_OK);

		} else if (strcmp(argv[1], "add_node") == 0){
            Node* node = (Node*)TclObject::lookup(argv[2]);
            
            nodes.push_back(node);
            data[node].pointer = std::string(argv[2]);

            tcl.resultf("%s", argv[2]);
            return TCL_OK;

        } else if (strcmp(argv[1], "set_mig_root") == 0){
            mig_root = (Node*)TclObject::lookup(argv[2]);
            return TCL_OK; 
            
        } else if (strcmp(argv[1], "set_traffic_src") == 0){
            traffic_src = (Node*)TclObject::lookup(argv[2]);
            return TCL_OK; 
            
        } else if (strcmp(argv[1], "set_simulator") == 0){
            sim_ptr = std::string(argv[2]);
            return TCL_OK;
        }      

    } else if (argc == 4) {
        if (strcmp(argv[1], "add_child") == 0){
            Node* parent = (Node*)TclObject::lookup(argv[2]);
            Node* child = (Node*)TclObject::lookup(argv[3]);
            
            connect_nodes(parent, child);

            return TCL_OK;

        } else if (strcmp(argv[1], "connect_agents") == 0) {
            Node* n1 = (Node*)TclObject::lookup(argv[2]);
            Node* n2 = (Node*)TclObject::lookup(argv[3]);

            connect_agents(n1, n2); 
            
            return TCL_OK; 

        } else if (strcmp(argv[1], "send_data") == 0) {
            Node* n1 = (Node*)TclObject::lookup(argv[2]);
            Node* n2 = (Node*)TclObject::lookup(argv[3]);
            
            send_data(n1);
            
            return TCL_OK; 
        } else if (strcmp(argv[1], "duplicate_tree") == 0) {
            Node* n1 = (Node*)TclObject::lookup(argv[2]);
            Node* n2 = (Node*)TclObject::lookup(argv[3]);
            
            duplicate_tree(n1, n2);
             
            return TCL_OK; 
        } 
    } else if (argc == 7){ 
		if (strcmp(argv[1], "activate_tunnel") == 0){
            
            auto in = (Node*)TclObject::lookup(argv[2]);
            auto out = (Node*)TclObject::lookup(argv[3]);
            auto from = (Node*)TclObject::lookup(argv[5]);
            auto to = (Node*)TclObject::lookup(argv[6]);
            
            int uid = mig_manager().activate_tunnel(in, out, 
                                                    from, to);

            tcl.resultf("%d", uid);

			return TCL_OK;
		}
	}
    return 0; 
}


void MyTopology::connect_agents(Node* n1, Node* n2){
    tcl_command({sim_ptr, "connect", 
                data[n1].tcp, data[n2].tcp_sink});
}


void MyTopology::connect_nodes(Node* parent, Node* child){
    data[parent].add_child(child); 
    data[child].add_parent(parent); 
    add_link(parent, child);
}


void MyTopology::setup_apps(){

    for(Node* node: nodes){
        // set up the agents
        tcl_command({"new Agent/UDP"});
        data[node].udp = Tcl::instance().result();
        tcl_command({"new Agent/TCP"});
        data[node].tcp = Tcl::instance().result();
        tcl_command({"new Agent/TCPSink"});
        data[node].tcp_sink = Tcl::instance().result();

        // connect the agent to the node 
        tcl_command({sim_ptr, "attach-agent",
                     data[node].pointer, 
                     data[node].udp});

        tcl_command({sim_ptr, "attach-agent",
                     data[node].pointer, 
                     data[node].tcp});

        tcl_command({sim_ptr, "attach-agent",
                     data[node].pointer, 
                     data[node].tcp_sink});

        // set up the application 
        tcl_command({"new Application"});
        data[node].app = Tcl::instance().result();

        // connect the app to the agent 
        tcl_command({data[node].app, "attach-agent", 
                        data[node].tcp});

        tcl_command({data[node].app, "start"});
    }
}

void MyTopology::notify_flow_fin(Node* n){
    auto& orch = BaseOrchestrator::instance();    
    orch.vm_precopy_finished(n);
    std::cout << "precopy finished";
}

void MyTopology::add_link(Node* parent, Node* child){
    tcl_command({sim_ptr, "duplex-link",
                 data[parent].pointer, 
                 data[child].pointer,
                 BW, DELAY, QUEUE_T});
}

Node* MyTopology::make_node(){
    tcl_command({sim_ptr, "node"});
    std::string node_ptr = Tcl::instance().result();

    Node* node = (Node*)TclObject::lookup(node_ptr.c_str());
    
    nodes.push_back(node);
    data[node].pointer = node_ptr;
    data[node].me = node; 
    
    return node; 
}

void MyTopology::make_tree(Node* root, 
                           std::vector<int> branching_ds){
    
    data[root].layer_from_bottom = branching_ds.size();

    // The first element shows the branching degree for 
    // this layer. 
    int current_branching = branching_ds[0];

    // remove the first element to get the branching degrees
    // for the bottom layer.
    std::vector<int> next_branching_ds = branching_ds;
    next_branching_ds.erase(next_branching_ds.begin());
    
    for(int i = 0; i < current_branching; i++){
        Node* child = make_node();
        connect_nodes(root, child); 
                
        if(next_branching_ds.size() > 0){
            make_tree(child, next_branching_ds);
        }
    }
}


Node* MyTopology::find_node(Node* root, 
                           std::vector<int> branch_ids){
    
    int current_id = branch_ids[0]; 
    Node* child = data[root].children[current_id];
    
    if (branch_ids.size() == 1){
        return child; 
    } else {
        std::vector<int> temp = branch_ids;
        temp.erase(temp.begin());
        return find_node(child, temp);
    }
}


void MyTopology::duplicate_tree(Node* src, Node* dst){
    make_peers(src, dst);
    std::stack<Node*> to_visit; 
    to_visit.push(src);

    while (to_visit.size() > 0){
        Node* current = to_visit.top();  
        to_visit.pop();

        if (current != src){
            Node* copy = make_node();
            make_peers(current, copy); 
            Node* parent = data[current].first_parent();
            Node* copy_parent = data[parent].peer;
            connect_nodes(copy_parent, copy);
        }

        auto children = data[current].children; 
        std::reverse(children.begin(),
                     children.end());

        for(Node* n: children){
            to_visit.push(n);
        }  
    }
}


std::vector<Node*> MyTopology::get_leaves(Node* root){
    std::vector<Node*> leaves; 
    std::stack<Node*> to_visit; 
    to_visit.push(root);

    while (to_visit.size() > 0){
        Node* current = to_visit.top();  
        to_visit.pop();

        if (data[current].children.size() == 0){
            leaves.push_back(current);
        }

        auto children = data[current].children; 
        std::reverse(children.begin(),
                     children.end());

        for(Node* n: children){
            to_visit.push(n);
        }
    }

    return leaves; 
}


std::vector<Node*> MyTopology::get_internal_nodes(Node* root){
    std::vector<Node*> internals; 
    std::stack<Node*> to_visit; 
    to_visit.push(root);

    while (to_visit.size() > 0){
        Node* current = to_visit.top();  
        to_visit.pop();

        if (data[current].children.size() != 0){
            internals.push_back(current);
        }

        auto children = data[current].children; 
        std::reverse(children.begin(),
                     children.end());

        for(Node* n: children){
            to_visit.push(n);
        }
    }

    return internals; 
}



bool compare_address(Node* n1, Node* n2){
    return (n1->address() < n2->address());
}

std::vector<Node*> MyTopology::get_all_siblings(Node* node, 
                                                Node* root){
    std::vector<Node*> siblings; 

    if (node == root){
        siblings.push_back(node); 
        return siblings; 
    } else {
        auto parent = data[node].first_parent(); 
        auto parent_siblings = get_all_siblings(parent, root);

        for (auto& p : parent_siblings){
            for (auto& c: data[p].children){
                siblings.push_back(c); 
            }
        }

        std::sort(siblings.begin(), siblings.end(), compare_address);

        return siblings;
    }
}

Node* MyTopology::get_next_sibling(Node* node, 
                                   Node* root){
    bool found_node = false; 

    for (auto& s: get_all_siblings(node, root)){
        if (found_node){
            return s; 
        } 
        if (s == node){
            found_node = true; 
        }
    }

    return nullptr; 
}


Node* MyTopology::get_nth_parent(Node* node, int n){
    auto nth_parent = node;

    for (int i = 0; i < n; i++){
        nth_parent = data[nth_parent].first_parent(); 
    } 

    return nth_parent; 
}


int MyTopology::get_subtree_depth(Node* root){
    int depth = 0; 

    Node* current_node = root; 

    while (data[current_node].children.size() > 0){
        current_node = data[current_node].children[0]; 
        depth ++; 
    }

    return depth; 
}

void MyTopology::process_packet(Packet* p, Handler*h, Node* node){
    data[node].process_packet(p, h);
};

void MyTopology::introduce_nodes_to_classifiers(){
    for (auto& node: nodes){
        node->introduce_to_classifer();
    }
}


void MyTopology::make_peers(Node* n1, Node* n2){
    data[n1].peer = n2; 
    data[n2].peer = n1; 
}


void MyTopology::print_nodes(){
    for(Node* node: nodes){ 
        std::cout << node << std::endl;
    }
}

void MyTopology::print_graph(){
    for(Node* node: nodes){
        std::cout << node->address() << ":" << " ";
        for(Node* child: data[node].children){
            std::cout << child->address() << " ";
        }
        std::cout << std::endl;
    }
}


void MyTopology::send_data(Node* n1){
    tcl_command({data[n1].app, "send", "1000"});
}
