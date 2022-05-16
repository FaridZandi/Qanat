#include "orchestrator.h"
#include "orch_v1.h"
#include "orch_v2.h"
#include "my_topology.h"
#include "node.h"
#include "utility.h"
#include <iostream>



BaseOrchestrator& BaseOrchestrator::instance(){
    // return OrchestratorV1::instance(); 
    return OrchestratorV2::instance(); 
    // return Orchestrator::instance(); 
};

BaseOrchestrator::BaseOrchestrator(){
    // parallel_migrations = MyTopology::parallel_mig; 
}

void BaseOrchestrator::setup_nodes(){
    auto& topo = MyTopology::instance(); 
    
    auto mig_root = topo.get_mig_root();
    auto mig_root_peer = topo.get_peer(mig_root);

    // The main tree is just working normally.
    for(auto& node: topo.get_all_nodes(mig_root)){
        mig_state[node] = MigState::Normal;
    }

    // The other tree is just out of service.
    for(auto& node: topo.get_all_nodes(mig_root_peer)){
        mig_state[node] = MigState::OutOfService;
    }

    for (auto& root: std::list<Node*>({mig_root, mig_root_peer})){
        //populate the VM NFs based on the list
        for(auto& node: topo.get_leaves(root)){    
            topo.get_data(node).mode = OpMode::VM;
            for (nf_spec& nfs: get_vm_nf_list()){
                topo.get_data(node).add_nf(nfs.type, nfs.parameter);
            }
        }

        //populate the GW NFs based on the list
        for(auto& node: topo.get_internals(root)){     
            topo.get_data(node).mode = OpMode::GW;
            for (nf_spec& nfs: get_gw_nf_list()){
                topo.get_data(node).add_nf(nfs.type, nfs.parameter);
            }
        }
    }

    topo.introduce_nodes_to_classifiers(); 

    // for(auto& node: topo.get_used_nodes()){
    //     auto path = topo.get_path(node, PATH_MODE_RECEIVER);
    //     std::cout << "to " << topo.uid(node) << ": ";
    //     for (auto p : path){
    //         std::cout << p << " ";
    //     }
    //     std::cout << std::endl; 

    //     path = topo.get_path(node, PATH_MODE_SENDER);
    //     std::cout << "from " << topo.uid(node) << ": ";
    //     for (auto p : path){
    //         std::cout << p << " ";
    //     }
    //     std::cout << std::endl; 
    // }
}


void BaseOrchestrator::initiate_data_transfer(
                            Node* node, int size, 
                            void (*callback) (Node*)){
    auto& topo = MyTopology::instance();

    topo.connect_agents(node, topo.get_peer(node));

    auto tcp_name = topo.get_data(node).tcp.c_str();
    Agent* agent = (Agent*)TclObject::lookup(tcp_name);
    agent->finish_notify_callback = callback; 
    agent->is_finish_callback_set = true;

    topo.send_data(node, size);
}


void BaseOrchestrator::tunnel_subtree_tru_parent(Node* node){
    auto& topo = MyTopology::instance();

    if (topo.get_mig_root() == node){
        topo.is_migration_finished = true;

        // to clear the cache 
        topo.get_path(NULL, PATH_MODE_SENDER, true); 
    } else {
        // setup tunnels for all the nodes in one layer up. 
        int node_layer = topo.get_data(node).layer_from_bottom;
        auto parent = topo.get_nth_parent(node, 1); 

        for(auto leaf: topo.get_leaves(node)){
            topo.setup_nth_layer_tunnel(leaf, node_layer + 1);
        }

        if (mig_state[parent] == MigState::Normal){
            set_node_state(parent, MigState::PreMig);
            set_peer_state(parent, MigState::PreMig);
            log_event("start gw precopy", parent);
        }

    }
}

void BaseOrchestrator::log_event(std::string message, Node* node, int arg, bool print_tree){
    auto& topo = MyTopology::instance();
    

    std::cout << std::endl; 
    std::cout << "---------------------------------------------"; 
    std::cout << std::endl; 
    print_time();

    std::cout << " "; 


    if (node != nullptr){
        auto& data = topo.get_data(node);
        
        std::cout << "[";

        if (data.mode == OpMode::VM) {
            std::cout << "VM";
        } else {
            std::cout << "GW";
        }

        std::cout << "-";
        std::cout << data.layer_from_bottom; 
        std::cout << "-";
        std::cout << data.uid; 
        std::cout << "-";
        std::cout << data.which_tree; 
        
        std::cout << "] ";  
    }

    if (arg != -1){
        std::cout << "[" << arg << "] ";  
    }

    std::cout << message; 

    std::cout << std::endl;
    std::cout << "---------------------------------------------"; 
    std::cout << std::endl;
    

    if (print_tree){
        topo.print_graph(true);
    }
}

void BaseOrchestrator::buffer_on_peer(Node* node){
    auto& topo = MyTopology::instance();
    auto peer = topo.get_peer(node);
    auto peer_data = topo.get_data(peer);

    if (topo.get_data(node).mode == VM){
        auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");
        log_event("start vm buffering", peer);
        peer_buffer->start_buffering();

    } else if (topo.get_data(node).mode == GW){
        auto peer_buffer = (PriorityBuffer*)peer_data.get_nf("pribuf");
        log_event("start gw buffering", peer);
        peer_buffer->start_buffering();
    }
}

void BaseOrchestrator::process_on_peer(Node* node){
    auto& topo = MyTopology::instance();
    auto peer = topo.get_peer(node);
    auto peer_data = topo.get_data(peer);

    if (topo.get_data(node).mode == VM){

        auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");

        auto queue_depth = peer_buffer->get_buffer_size();

        // log_event("releasing a buffer of size ", queue_depth, false);
        log_event("end vm buffering", peer);

        peer_buffer->stop_buffering();

    } else if (topo.get_data(node).mode == GW){

        auto peer_buffer = (PriorityBuffer*)peer_data.get_nf("pribuf");

        auto queue_depth_high = peer_buffer->get_buffer_size_highprio();
        auto queue_depth_low = peer_buffer->get_buffer_size_lowprio();


        // log_event("releasing a high prio buffer of size ", queue_depth_high, false);
        // log_event("releasing a low prio buffer of size ", queue_depth_low, false);
        log_event("end gw buffering", peer);

        peer_buffer->stop_buffering();
    }
}


double BaseOrchestrator::random_wait(){
    int r = 80 + std::rand() % 40;
    double wait = (double) r / 100.0; 
    return 0; 
}

MigState BaseOrchestrator::get_mig_state(Node* node){
    return mig_state[node];
}

std::string BaseOrchestrator::get_mig_state_string(Node* node){
    auto state = mig_state[node];
    switch(state){
        case MigState::InMig: 
            return "\e[101mInMig\e[0m";
        case MigState::PreMig: 
            return "\e[43mPreMg\e[0m";
        case MigState::Migrated: 
            return "\e[42mMgrtd\e[0m";
        case MigState::Buffering: 
            return "\e[41mBuFFr\e[0m";
        case MigState::Normal: 
            return "\e[44mNorml\e[0m";
        case MigState::OutOfService: 
            return "\e[107m\e[30mzzzzz\e[0m";
    }
}


void BaseOrchestrator::set_node_state(Node* node, MigState state){
    mig_state[node] = state; 
}

void BaseOrchestrator::set_peer_state(Node* node, MigState state){
    auto& topo = MyTopology::instance();
    auto peer = topo.get_peer(node);
    mig_state[peer] = state; 
}

Orchestrator::Orchestrator() {
    
}

Orchestrator::~Orchestrator(){

}


