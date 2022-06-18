#include "orchestrator.h"
#include "orch_bottom_up.h"
#include "orch_top_down.h"
#include "orch_random.h"
#include "my_topology.h"
#include "node.h"
#include "utility.h"
#include <iostream>



BaseOrchestrator& BaseOrchestrator::instance(){
    auto& topo = MyTopology::instance();

    if(topo.orch_type == 1) {
        return OrchBottomUp::instance(); 
    } else if (topo.orch_type == 2) {
        return OrchTopDown::instance(); 
    } else if (topo.orch_type == 3) {
        return OrchRandom::instance(); 
    } else {
        std::cout << "Please specify the orchestrator type you wish to use.";
        std::cout << std::endl; 
        std::cout << "Exitting..."; 
        std::cout << std::endl; 
        exit(1); 
    }
};

BaseOrchestrator::BaseOrchestrator(){
    in_migration_vms = 0;
    in_migration_nodes = 0;
    in_migration_gws = 0;   
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
                auto nf = topo.get_data(node).add_nf(nfs.type, nfs.parameter);
                if (nfs.type == "buffer") {
                    auto buf = (Buffer*) nf;
                    buf->set_rate(833333 * 2);
                }
            }
        }

        //populate the GW NFs based on the list
        for(auto& node: topo.get_internals(root)){     
            topo.get_data(node).mode = OpMode::GW;
            for (nf_spec& nfs: get_gw_nf_list()){
                // std::cout << "Adding nf: " << nfs.type << " to ";
                // std::cout << " uid: " << topo.get_data(node).uid << std::endl;
                topo.get_data(node).add_nf(nfs.type, nfs.parameter);
            }
        }
    }

    topo.introduce_nodes_to_classifiers(); 
}

void BaseOrchestrator::initiate_data_transfer(
                        Node* node, int size, 
                        void (*callback) (Node*)){
                                
    auto& topo = MyTopology::instance();

    topo.connect_agents(node, topo.get_peer(node));

    std::cout << "Initiating data transfer from " << topo.uid(node) << " to ";
    std::cout << topo.uid(topo.get_peer(node));
    std::cout << " of size " << size << std::endl;


    auto tcp_name = topo.get_data(node).tcp.c_str();
    Agent* agent = (Agent*)TclObject::lookup(tcp_name);
    agent->finish_notify_callback = callback; 
    agent->is_finish_callback_set = true;

    topo.send_data(node, size);
}




void BaseOrchestrator::log_event(std::string message, Node* node, int arg, bool print_tree){
    auto& topo = MyTopology::instance();
    

    std::cout << std::endl; 
    std::cout << "---------------------------------------------"; 
    std::cout << std::endl; 

    std::cout << "protocol_event ";
    print_time();

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
        log_event("end vm buffering", peer);
        peer_buffer->stop_buffering();
        
    } else if (topo.get_data(node).mode == GW){
        auto peer_buffer = (PriorityBuffer*)peer_data.get_nf("pribuf");
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


bool BaseOrchestrator::is_gateway(Node* node){
    auto& topo = MyTopology::instance();
    if (topo.get_data(node).mode == OpMode::GW){
        return true;
    } else {
        return false;
    }
}
