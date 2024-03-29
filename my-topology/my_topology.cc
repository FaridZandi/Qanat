#include "my_topology.h"
#include "mig_manager.h"
#include "orchestrator.h"
#include "tcp-full.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>    
#include <stack> 
#include "utility.h"
#include <chrono>
#include <ctime>  


static class MyTopologyClass : public TclClass {
public:
	MyTopologyClass() : TclClass("MyTopology") {}
	TclObject* create(int, const char*const*) {
		return (new MyTopology());
	}
} my_class_topology;

MyTopology* MyTopology::instance_ = nullptr;
int MyTopology::toponode_uid_counter = 0;

int MyTopology::verbose = false;
int MyTopology::verbose_mig = 0;
int MyTopology::verbose_nf = 0;
int MyTopology::vm_precopy_size = 0;
int MyTopology::vm_snapshot_size = 0;
int MyTopology::gw_snapshot_size = 0;
int MyTopology::parallel_mig = 0;
int MyTopology::orch_type = 0;
int MyTopology::prioritization_level = 0;
int MyTopology::process_after_migration = 0;
double MyTopology::stat_record_interval = 0;


MyTopology::MyTopology(){
    bind_bool("verbose_", &verbose); 

    bind("verbose_mig_", &verbose_mig); 
    bind("verbose_nf_", &verbose_nf); 
    bind("vm_precopy_size_", &vm_precopy_size); 
    bind("vm_snapshot_size_", &vm_snapshot_size); 
    bind("gw_snapshot_size_", &gw_snapshot_size);
    bind("parallel_mig_", &parallel_mig);
    bind("process_after_migration_", &process_after_migration);
    bind("prioritization_level_", &prioritization_level);
    bind("orch_type_", &orch_type);
    bind("stat_record_interval_", &stat_record_interval);

    is_migration_started = false;
    is_migration_finished = false; 
    is_sent_traffic_to_dest = false;
    is_node_setup_done = false;

    mig_manager_ = new MigrationManager(); 
    instance_ = this; 

    mig_root = nullptr; 
    stat_recorder = nullptr; 

    tunnelled_packets = 0; 
}

MyTopology::~MyTopology(){
	delete mig_manager_; 

    if (stat_recorder != nullptr){
        delete stat_recorder; 
    }
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
        int root = atoi(argv[2]);
        std::vector<int> branch_ids;
        for(int i = 3; i < argc; i++){
            branch_ids.push_back(atoi(argv[i]) - 1);
        } 
        int node_uid = find_node(root, branch_ids);
        tcl.resultf("%s", data[node[node_uid]].pointer.c_str());
        return TCL_OK;

    } else if (strcmp(argv[1], "make_tree") == 0) {
        std::vector<int> branching_ds;
        for(int i = 2; i < argc; i++){
            branching_ds.push_back(atoi(argv[i]));
        } 
        int root_uid = make_tree(-1, branching_ds);
        mig_root = node[root_uid];
        tcl.resultf("%s", data[mig_root].pointer.c_str());
        return TCL_OK;  

    } else if (argc == 2) {
        if (strcmp(argv[1], "setup_nodes") == 0){
            auto& orch = BaseOrchestrator::instance();
            is_node_setup_done = true; 
            orch.setup_nodes();
            return TCL_OK; 

        } else if (strcmp(argv[1], "start_stat_record") == 0){
            stat_recorder = new StatRecorder(); 
            stat_recorder->interval = stat_record_interval; 
            stat_recorder->start(); 
            return TCL_OK; 

        } else if (strcmp(argv[1], "start_migration") == 0){
            auto& orch = BaseOrchestrator::instance();
            orch.start_migration();
            is_migration_started = true; 
            return TCL_OK; 

        } else if (strcmp(argv[1], "print_nodes") == 0) {
            print_nodes();
            return TCL_OK; 

        } else if (strcmp(argv[1], "print_graph") == 0) {
            print_graph();
            return TCL_OK; 

        } else if (strcmp(argv[1], "duplicate_tree") == 0) {
            int n1 = data[mig_root].uid; 
            duplicate_tree(n1);
            return TCL_OK; 

        } else if (strcmp(argv[1], "print_stats") == 0) {
            print_stats();
            return TCL_OK; 
        } 
    } else if (argc == 3) {
        if (strcmp(argv[1], "deactivate_tunnel") == 0) { 
			int uid = atoi(argv[2]);
            mig_manager().deactivate_tunnel(uid); 
			return(TCL_OK);

		} else if (strcmp(argv[1], "add_node") == 0){
            Node* this_node = make_node(true);
            tcl.resultf("%s", data[this_node].uid);
            return TCL_OK;

        } else if (strcmp(argv[1], "set_simulator") == 0){
            sim_ptr = std::string(argv[2]);
            return TCL_OK;

        } else if (strcmp(argv[1], "add_node_to_source") == 0){
            auto new_node = (Node*)TclObject::lookup(argv[2]);
            source_nodes_ptrs.push(argv[2]);
            source_nodes.push(new_node);
            return TCL_OK;

        } else if (strcmp(argv[1], "add_node_to_dest") == 0){
            auto new_node = (Node*)TclObject::lookup(argv[2]);
            dest_nodes_ptrs.push(argv[2]);
            dest_nodes.push(new_node);
            return TCL_OK; 
            
        } else if (strcmp(argv[1], "get_logical_leaf") == 0){
			int leaf_id = atoi(argv[2]);
            auto children = get_leaves(mig_root);
            if(leaf_id >= children.size()){
                tcl.resultf("%d", -1);
            } else {
                tcl.resultf("%s", data[children[leaf_id]].pointer.c_str());
            }
            return TCL_OK;
        }             
    } else if (argc == 4) {
        if (strcmp(argv[1], "add_child") == 0){
            Node* parent = (Node*)TclObject::lookup(argv[2]);
            Node* child = (Node*)TclObject::lookup(argv[3]);
            connect_nodes(data[parent].uid, data[child].uid);
            return TCL_OK;

        } else if (strcmp(argv[1], "connect_agents") == 0) {
            Node* n1 = (Node*)TclObject::lookup(argv[2]);
            Node* n2 = (Node*)TclObject::lookup(argv[3]);
            connect_agents(n1, n2); 
            return TCL_OK; 

        }  else if (strcmp(argv[1], "rate_limit_node") == 0){
            auto limited_node = (Node*)TclObject::lookup(argv[2]);
            int rate = atoi(argv[3]);
            limited_node->introduce_to_classifer();      
            data[limited_node].add_nf("rate_limiter", rate); 
            data[limited_node].uid = toponode_uid_counter; 
            data[limited_node].node = limited_node; 
            data[limited_node].peer = -1; 
            data[limited_node].pointer = argv[2];
        
            toponode_uid_counter += 1;
            return TCL_OK;    
        }
    } else if (argc == 5) { 
        if (strcmp(argv[1], "send_data") == 0) {
            Node* n1 = (Node*)TclObject::lookup(argv[2]);
            Node* n2 = (Node*)TclObject::lookup(argv[3]);
            int size = atoi(argv[4]);
            connect_agents(n1, n2);
            send_data(n1, size);
            return TCL_OK; 
        } 
    } else if (argc == 6){ 
		if (strcmp(argv[1], "activate_tunnel") == 0){
            auto in = (Node*)TclObject::lookup(argv[2]);
            auto out = (Node*)TclObject::lookup(argv[3]);
            auto from = (Node*)TclObject::lookup(argv[4]);
            auto to = (Node*)TclObject::lookup(argv[5]);
            int uid = mig_manager().activate_tunnel(in, out, 
                                                from, to);

            std::cout << "activating tunnel" << std::endl; 
            tcl.resultf("%d", uid);
			return TCL_OK;
		}
	}
    return 0; 
}

void MyTopology::start_tcp_app(Node* n1){
    tcl_command({"new Agent/TCP/FullTcp"});
    data[n1].tcp = Tcl::instance().result();
    
    auto agent = (FullTcpAgent*)TclObject::lookup(
        data[n1].tcp.c_str()
    );

    agent->set_traffic_class(2);
    agent->node = n1;

    tcl_command({data[n1].tcp, "set",
                "minrto_", "0.05",});

    tcl_command({sim_ptr, "attach-agent",
                 data[n1].pointer, 
                 data[n1].tcp});

    // set up the application 
    tcl_command({"new Application"});
    data[n1].app = Tcl::instance().result();

    // connect the app to the agent 
    tcl_command({data[n1].app, "attach-agent", data[n1].tcp});
}

void MyTopology::connect_agents(Node* n1, Node* n2){
    static int connection_counter = 1000000;    

    auto conn_c = std::to_string(connection_counter); 

    start_tcp_app(n1);
    start_tcp_app(n2);

    tcl_command({sim_ptr, "connect", data[n1].tcp, data[n2].tcp});
    tcl_command({data[n2].tcp, "listen"});
    tcl_command({data[n1].tcp, "set", "fid_", conn_c});
    tcl_command({data[n2].tcp, "set", "fid_", conn_c});

    // stat_recorder->tracked_fids.push_back(connection_counter);
    // auto r_agent = (FullTcpAgent*)TclObject::lookup(
    //     data[n2].tcp.c_str()
    // );
    // stat_recorder->fid_agent_map[connection_counter] = r_agent;

    connection_counter++;
}


void MyTopology::connect_nodes(int parent, int child){
    data[node[parent]].add_child(child); 
    data[node[child]].add_parent(parent); 
}

Node* MyTopology::make_node(bool is_source){
    Node* this_node;
    std::string this_node_ptr; 

    if (is_source == true){
        this_node = source_nodes.top();
        source_nodes.pop(); 
        this_node_ptr = source_nodes_ptrs.top(); 
        source_nodes_ptrs.pop();
    } else {
        this_node = dest_nodes.top();
        dest_nodes.pop(); 
        this_node_ptr = dest_nodes_ptrs.top(); 
        dest_nodes_ptrs.pop();
    }

    used_nodes.push_back(this_node);
    data[this_node].node = this_node; 
    data[this_node].peer = -1; 
    data[this_node].pointer = this_node_ptr;
    data[this_node].uid = toponode_uid_counter;

    node[toponode_uid_counter] = this_node;

    toponode_uid_counter ++; 
    
    return this_node; 
}

int MyTopology::make_tree(int parent, std::vector<int> branching_ds, int child_index){
    // assign a virtual identity to a node.
    Node* this_node = make_node(true);
    int this_uid = data[this_node].uid;

    // Setting the depth of this node in the virtual tree. 
    data[this_node].layer_from_bottom = branching_ds.size();
    data[this_node].child_index = child_index;
    data[this_node].which_tree = 0;

    // If there is a parent, connect it to this node.
    if(parent != -1){
        connect_nodes(data[node[parent]].uid, this_uid);
    }

    // This was the last level of the tree. nothing more to do. 
    if (branching_ds.size() == 0){
        return -1; 
    }

    // Remove the first element to get the branching degrees
    // for the bottom layer.
    std::vector<int> next_branching_ds = branching_ds;
    next_branching_ds.erase(next_branching_ds.begin());

    // The first element shows the branching degree for 
    // this layer. 
    int current_branching = branching_ds[0];
    for(int i = 0; i < current_branching; i++){        
        make_tree(this_uid, next_branching_ds, i);
    }

    return this_uid;
}


int MyTopology::find_node(int root, 
                          std::vector<int> branch_ids){

    int current_id = branch_ids[0]; 

    auto this_node = node[root];
    int child = data[this_node].children[current_id];
    
    if (branch_ids.size() == 1){
        return child; 
    } else {
        std::vector<int> temp = branch_ids;
        temp.erase(temp.begin());
        return find_node(child, temp);
    }
}


int MyTopology::duplicate_tree(int root){
    Node* root_peer = make_node(false); 
    
    data[root_peer].layer_from_bottom = data[node[root]].layer_from_bottom;
    data[root_peer].child_index = data[node[root]].child_index;
    data[root_peer].which_tree = 1;

    int root_peer_uid = data[root_peer].uid;
    make_peers(root, root_peer_uid);

    std::stack<int> to_visit; 
    to_visit.push(root);

    while (to_visit.size() > 0){
        int current = to_visit.top();  
        to_visit.pop();

        if (current != root){
            Node* copy = make_node(false);

            data[copy].layer_from_bottom = data[node[current]].layer_from_bottom;
            data[copy].child_index = data[node[current]].child_index;
            data[copy].which_tree = 1;
            
            make_peers(current, data[copy].uid); 

            int parent_pid = data[node[current]].first_parent();
            int parent_peer_uid = data[node[parent_pid]].peer;
            connect_nodes(parent_peer_uid, data[copy].uid);
        }

        auto children = data[node[current]].children; 
        std::reverse(children.begin(),
                     children.end());

        for(auto n: children){
            to_visit.push(n);
        }  
    }
    return root_peer_uid; 
}


std::vector<Node*> MyTopology::get_subtree_nodes( 
        Node* root, 
        bool include_leaves, 
        bool include_internals) {

    std::vector<Node*> subtree_nodes; 

    std::stack<int> to_visit; 
    to_visit.push(data[root].uid);

    while (to_visit.size() > 0){
        int current = to_visit.top();  
        to_visit.pop();

        if (data[node[current]].children.size() == 0){
            if (include_leaves) {
                subtree_nodes.push_back(node[current]);
            }
        } else {
            if (include_internals) {
                subtree_nodes.push_back(node[current]);
            }
        }

        auto children = data[node[current]].children; 
        std::reverse(children.begin(),
                     children.end());

        for(int n: children){
            to_visit.push(n);
        }
    }

    return subtree_nodes; 
}

Node* MyTopology::get_first_child(Node* n){
    auto children = data[n].children;
    return node[children[0]];
}


Node* MyTopology::get_next_sibling(Node* n){

    auto node_id = data[n].uid;

    int parent_pid = data[n].first_parent();
    if (parent_pid == -1){
        return nullptr;
    } 

    auto children = data[node[parent_pid]].children;
    
    for (int i = 0; i < children.size(); i++){
        if (children[i] == node_id){
            if (i == children.size() - 1){
                return nullptr;
            } else {
                return node[children[i+1]];
            }
        }
    }
}

Node* MyTopology::get_nth_parent(Node* this_node, int n){
    auto nth_parent = this_node;
    // std::cout << "getting the " << n << "th parent of " << data[nth_parent].uid << std::endl;
    for (int i = 0; i < n; i++){
        // std::cout << i << "th parent: " << data[nth_parent].uid << std::endl;
        int parent_id = data[nth_parent].first_parent(); 
        nth_parent = node[parent_id];
    } 
    // std::cout << "returning " << nth_parent << std::endl;
    return nth_parent; 
}

void MyTopology::process_packet(Packet* p, Handler*h, Node* node){

    hdr_ip* iph = hdr_ip::access(p);

    // std::cout << "iph->dst_.addr_: " << iph->dst_.addr_ << " "; 
    // std::cout << "node->address() " << node->address();
    // std::cout << std::endl;  

    // ignore class 2 traffic 
    if (iph->traffic_class == 2){
        node->get_classifier()->recv2(p, h);
        return;  
    }

    // std::cout << "is tcp? " << iph->is_tcp_traffic << " and class: " << iph->traffic_class << std::endl;
    // allow in only the TCP traffic or non-TCP traffic with traffic_class == 3
    if(not iph->is_tcp_traffic){
        if (iph->traffic_class != 3){
            node->get_classifier()->recv2(p, h); 
            return; 
        }
    }

    if(iph->dst_.addr_ == node->address() or 
       iph->src_.addr_ == node->address()){

        data[node].process_packet(p, h);
    }
};

void MyTopology::introduce_nodes_to_classifiers(){
    for (auto& node: used_nodes){
        node->introduce_to_classifer();
    }
}

std::vector<Node*> MyTopology::get_children(Node* n){
    std::vector<Node*> children; 

    for(int child: data[n].children){
        children.push_back(node[child]);
    }
    return children; 
}; 

Node* MyTopology::get_peer(Node* n){
    int peer_uid = data[n].peer;
    return node[peer_uid]; 
}

Node* MyTopology::get_node_by_address(int addr){
    // the cache for storing the mappings
    static std::map<int, Node*> address_to_node; 

    if (address_to_node.find(addr) != address_to_node.end()) {
        return address_to_node[addr];
    } else {
        for(auto n : used_nodes){
            if (n->address() == addr){
                address_to_node[addr] = n; 
                return n; 
            }
        }
        address_to_node[addr] = nullptr;
        return nullptr;
    }
}

void MyTopology::inc_tunnelled_packets(){
    tunnelled_packets ++;
}

void MyTopology::migration_finished(){
    is_migration_finished = true;
    migration_finish_time = Scheduler::instance().clock();
}

void MyTopology::sent_traffic_to_dest(){
    is_sent_traffic_to_dest = true;
    clear_path_cache(); 
}
     
TopoNode& MyTopology::get_data(Node* node){
    return data[node];
} 

int MyTopology::uid(Node* node){
    if (node == nullptr){
        return -1; 
    } else {
        return data[node].uid; 
    }
} 

Node* MyTopology::get_mig_root(){
    return mig_root; 
}

std::vector<Node*>& MyTopology::get_used_nodes(){
    return used_nodes; 
}


std::vector<int> MyTopology::get_path(Node* n1, path_mode pm){
    auto dest = std::make_pair(n1, pm); 

    if (path_cache.find(dest) != path_cache.end()) {
        return path_cache[dest]; 
    }

    std::vector<int> result; 

    if (n1 != nullptr){
        // if the migration is finished, we should send the 
        // traffic to the destination zone directly. 
        if (is_sent_traffic_to_dest){
            n1 = get_peer(n1);
        }

        auto current = n1; 
        result.push_back(current->address());

        while (data[current].first_parent() != -1){
            int parent = data[current].first_parent(); 
            current = node[parent];
            result.push_back(current->address());
        }
    }

    if(pm == PATH_MODE_RECEIVER) { 
        std::reverse(result.begin(),
                     result.end());
    }  

    // print the path to the node the first time it is requested
    std::cout << "path to " << uid(n1) << ": ";
    for (auto n: result){
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // store the path in the cache and return it
    path_cache[dest] = result; 
    return result; 
}

void MyTopology::clear_path_cache(){
    path_cache.clear(); 
}

void MyTopology::make_peers(int n1, int n2){
    data[node[n1]].peer = n2; 
    data[node[n2]].peer = n1; 
}


void MyTopology::print_nodes(){
    for(Node* this_node: used_nodes){ 
        std::cout << this_node << std::endl;
    }
}


void MyTopology::print_tree_node(std::string prefix, 
                                 Node* this_node, 
                                 bool isLast, 
                                 bool print_state)  {

    auto& orch = BaseOrchestrator::instance();

    if(this_node != nullptr){
        std::cout << prefix;
        std::cout << (isLast ? "└──────" : "├──────");
        std::cout << this_node->address(); 
        std::cout << "("; 
        std::cout << data[this_node].uid; 
        std::cout << ")"; 
        if (print_state){
            std::cout << "(";
            std::cout << orch.get_mig_state_string(this_node);
            std::cout << ")";
        } else {
            std::cout << "(" << setw(2);
            std::cout << data[this_node].uid << ")" << " ";
        }
        std::cout << std::endl; 
        
        int child_count = data[this_node].children.size(); 

        for(int i = 0; i < child_count; i++){

            Node* child = node[data[this_node].children[i]];

            if (i == child_count - 1){
                print_tree_node(
                    prefix + (isLast ? "        " : "│       "), 
                    child, true, print_state
                );
            } else {
                print_tree_node(
                    prefix + (isLast ? "        " : "│       "), 
                    child, false, print_state
                );
            }
        }
    }
}

void MyTopology::print_graph(bool print_state){
    std::cout << "main tree: " << std::endl; 

    print_tree_node("", mig_root, true, print_state); 

    std::cout << "other tree: " << std::endl; 

    print_tree_node("", node[data[mig_root].peer], true, print_state); 
}


void MyTopology::send_data(Node* n1, int n_bytes){
    tcl_command({data[n1].app, "send", std::to_string(n_bytes)});
    // tcl_command({data[n1].app, "advance_bytes", std::to_string(n_bytes)});
}

std::vector<Node*> MyTopology::get_leaves(Node* root){
    return get_subtree_nodes(root, true, false);
}

std::vector<Node*> MyTopology::get_internals(Node* root){
    return get_subtree_nodes(root, false, true);
}

std::vector<Node*> MyTopology::get_all_nodes(Node* root){
    return get_subtree_nodes(root, true, true);
}


void MyTopology::start_recording_stats(int fid,
                                       FullTcpAgent* agent){
    stat_recorder->tracked_fids.push_back(fid); 
    stat_recorder->fid_agent_map[fid] = agent;
}


void MyTopology::print_stats(){

    std::cout << "----------------------------------" << std::endl;
    std::cout << "--------------Stats---------------" << std::endl;
    std::cout << "----------------------------------" << std::endl;

    if (stat_recorder != nullptr){
        stat_recorder->print_stats();
    }

    std::cout << "----------------------------------" << std::endl;
    std::cout << "----------------------------------" << std::endl;
}



void MyTopology::setup_nth_layer_tunnel(Node* node, int layer){
    auto peer = get_peer(node); 

    auto nth_parent = get_nth_parent(node, layer); 
    auto nth_parent_peer = get_peer(nth_parent); 

    // print_time(); 

    if (verbose) {
        std::cout << "node " << node->address() << " ";                           
        std::cout << "migrated to " << peer->address() << " ";                           
        std::cout << "tunnelled from " << nth_parent->address() << " ";
        std::cout << "to " << nth_parent_peer->address() << " ";
        std::cout << std::endl;
    }

    mig_manager().activate_tunnel(
        nth_parent, nth_parent_peer,  
        node, peer
    );
};



/**********************************************************
 * StatRecorder Implementation                            *  
 *********************************************************/


StatRecorder::StatRecorder(){

}

StatRecorder::~StatRecorder(){
    
}


void StatRecorder::record_stats(){
    auto& topo = MyTopology::instance(); 

    auto mig_root = topo.get_mig_root(); 
    auto mig_root_peer = topo.get_peer(mig_root);

    auto now = Scheduler::instance().clock();


    // record the stats for the internal nodes of the tree 
    for (auto& root: std::list<Node*>({mig_root, mig_root_peer})){
        for(auto& node: topo.get_internals(root)){

            auto pb = (PriorityBuffer*)(topo.get_data(node).get_nf("pribuf"));
            int high_prio_buf = pb->max_high_prio_buf_size; 
            int low_prio_buf = pb->max_low_prio_buf_size; 

            pb->max_high_prio_buf_size = pb->hp_q->length(); 
            pb->max_low_prio_buf_size = pb->lp_q->length();


            auto m = (Monitor*)(topo.get_data(node).get_nf("monitr"));
            int packet_count = m->get_packet_count(); 
            auto ooo_packet_count = m->get_ooo_packet_count(); 

            auto uid = topo.uid(node); 


            node_stats[uid][now] = {
                high_prio_buf, 
                low_prio_buf, 
                packet_count,
                ooo_packet_count,
            }; 
        } 
    }


    // record the stats for the internal nodes of the tree 
    for (auto& root: std::list<Node*>({mig_root, mig_root_peer})){
        for(auto& node: topo.get_leaves(root)){

            auto buffer = (Buffer*)(topo.get_data(node).get_nf("buffer"));
            int buf_size = buffer->max_buf_size; 

            buffer->max_buf_size = buffer->pq->length(); 

            auto m = (Monitor*)(topo.get_data(node).get_nf("monitr"));
            int packet_count = m->get_packet_count(); 

            auto uid = topo.uid(node); 

            node_stats[uid][now] = {0,buf_size, packet_count, 0}; 
        } 
    }

    

    // record the stats for the VM flows 
    for(auto& fid: tracked_fids){
        auto agent = fid_agent_map[fid];

        auto packets_received = agent->get_packets_received_count();
        auto total_in_flight_time = agent->get_total_in_flight_time();
        auto total_buffered_time = agent->get_total_buffered_time();

        double average_in_flight_time = 0;
        if (packets_received > 0){
            average_in_flight_time = total_in_flight_time / packets_received;
        }

        double average_buffered_time = 0;
        if (packets_received > 0){
            average_buffered_time = total_buffered_time / packets_received;
        }
        
        flow_stats[fid][now] = {
            packets_received,
            average_in_flight_time,
            average_buffered_time,
        };
    }

    // record the number of tunnelled packets
    tunnelled_packets[now] = MyTopology::instance().tunnelled_packets;
}


void StatRecorder::print_stats(){
    for (auto& node_stat: node_stats){
        for (auto& stat: node_stat.second){
            std::cout << "node_stats_recorder ";
            std::cout << "[";
            std::cout << node_stat.first; 
            std::cout << "]";

            std::cout << " ";

            std::cout << "[";
            std::cout << stat.first; 
            std::cout << "]";

            std::cout << " ";

            std::cout << stat.second.high_prio_buf;
            std::cout << " ";
            std::cout << stat.second.low_prio_buf;
            std::cout << " ";
            std::cout << stat.second.packet_count;
            std::cout << " ";
            std::cout << stat.second.ooo_packet_count;
            
            std::cout << std::endl; 
        }
    }


    for (auto& flow_stat: flow_stats){
        for (auto& stat: flow_stat.second){
            std::cout << "flow_stats_recorder ";
            std::cout << "[";
            std::cout << flow_stat.first; 
            std::cout << "]";

            std::cout << " ";

            std::cout << "[";
            std::cout << stat.first; 
            std::cout << "]";

            std::cout << " ";

            std::cout << stat.second.received_packet_count;
            std::cout << " ";
            std::cout << int(stat.second.average_in_flight_time * 1000000);
            std::cout << " ";
            std::cout << int(stat.second.average_buffered_time * 1000000);
            
            std::cout << std::endl; 
        }
    }

    for (auto& t: tunnelled_packets){
        std::cout << "tunnelled_packets_recorder ";
        std::cout << "[";
        std::cout << t.first; 
        std::cout << "]";

        std::cout << " ";
    
        std::cout << t.second;

        std::cout << std::endl;
    }

}

void StatRecorder::start(){
    record_stats(); 
    
    Event* e = new Event; 
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, interval);
}

void StatRecorder::handle(Event* event){
    record_stats(); 

    auto& topo = MyTopology::instance(); 
    auto now = Scheduler::instance().clock(); 
    auto stat_record_finish = topo.migration_finish_time + record_after_finish;

    if(topo.is_migration_finished && now > stat_record_finish){
        return; 
    } else if (now >= 12) {
        return; 
    } else {
        Event* e = new Event; 
        auto& sched = Scheduler::instance();
        sched.schedule(this, e, interval);
    }
}
