#include "orch_v2.h"
#include "orchestrator.h"
#include "my_topology.h"
#include "mig_manager.h"
#include "tcp-sink.h"
#include "node.h"
#include <iostream>
#include <sstream>
#include <algorithm>    
#include <stack> 
#include "nf.h"
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include "utility.h"


OrchestratorV2::OrchestratorV2() {

}

OrchestratorV2::~OrchestratorV2(){

}



void OrchestratorV2::setup_nodes(){
    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.get_mig_root();
    auto mig_root_peer = topo.get_peer(mig_root);

    for(auto& node: topo.get_all_nodes(mig_root)){
        mig_state[node] = MigState::NoMigState;
    }

    for (auto& root: std::list<Node*>({mig_root, mig_root_peer})){
        for(auto& node: topo.get_leaves(root)){    

            topo.get_data(node).mode = OpMode::VM;
            topo.get_data(node).add_nf("buffer", 100);
            // topo.get_data(node).add_nf("rate_limiter", 100);
            // topo.get_data(node).add_nf("delayer", 0.01);
            topo.get_data(node).add_nf("tunnel_manager");
            topo.get_data(node).add_nf("router");
            topo.get_data(node).print_nfs(); 
        }

        for(auto& node: topo.get_internals(root)){     
               
            topo.get_data(node).mode = OpMode::GW;
            topo.get_data(node).add_nf("monitor");
            // topo.get_data(node).add_nf("delayer", 0.01);
            topo.get_data(node).add_nf("tunnel_manager");
            topo.get_data(node).add_nf("router");
            topo.get_data(node).print_nfs(); 
        }
    }

    std::cout << "--------------------------" << std::endl;
    std::cout << "--------------------------" << std::endl;
    std::cout << "--------------------------" << std::endl;

    topo.introduce_nodes_to_classifiers(); 

    for(auto& node: topo.get_used_nodes()){
        auto path = topo.get_path(node, PATH_MODE_RECEIVER);
        std::cout << "to " << topo.uid(node) << ": ";
        for (auto p : path){
            std::cout << p << " ";
        }
        std::cout << std::endl; 

        path = topo.get_path(node, PATH_MODE_SENDER);
        std::cout << "from " << topo.uid(node) << ": ";
        for (auto p : path){
            std::cout << p << " ";
        }
        std::cout << std::endl; 
    }
};


void OrchestratorV2::start_migration(){
    std::srand(123);

    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.get_mig_root(); 

    std::cout << "VM migration Queue: ";

    for (auto& leaf: topo.get_leaves(mig_root)){
        vm_migration_queue.push(leaf);
        std::cout << leaf->address() << " ";
    }
    std::cout << std::endl;

    for(int i = 0; i < parallel_migrations; i++){
        if(vm_migration_queue.size() > 0){

            auto node = vm_migration_queue.front(); 
            vm_migration_queue.pop();
            start_vm_precopy(node);
        }
    }
};
    
  
void OrchestratorV2::start_vm_precopy(Node* vm){
    print_time(); 
    std::cout << "sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    mig_state[vm] = MigState::PreMig; 

    initiate_data_transfer(
        vm, 100000000, 
        [](Node* n){BaseOrchestrator::instance().vm_precopy_finished(n);}
    );
}

void OrchestratorV2::vm_precopy_finished(Node* vm){
    print_time(); 
    std::cout << "finished sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    start_vm_migration(vm); 
}; 


void OrchestratorV2::start_vm_migration(Node* vm){
    mig_state[vm] = MigState::InMig; 

    auto& topo = MyTopology::instance();
    auto peer = topo.get_peer(vm);
    auto peer_data = topo.get_data(peer);
    auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");
    
    topo.setup_nth_layer_tunnel(vm, 1); 

    print_time(); 
    std::cout << "start buffering for ";
    std::cout << peer->address() << std::endl; 
    peer_buffer->start_buffering();

    print_time(); 
    std::cout << "sending the VM snapshot for node: ";
    std::cout << vm->address() << std::endl;

    initiate_data_transfer(
        vm, 10000000, 
        [](Node* n){BaseOrchestrator::instance().vm_migration_finished(n);}
    );
}


void OrchestratorV2::vm_migration_finished(Node* vm){
    print_time(); 
    std::cout << "VM migration finished for: ";
    std::cout << vm->address() << std::endl;
    
    mig_state[vm] = MigState::Migrated; 

    auto& topo = MyTopology::instance();

    auto peer = topo.get_peer(vm);
    auto peer_data = topo.get_data(peer);
    auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");

    print_time(); 
    std::cout << "stopped buffering for ";
    std::cout << peer->address() << std::endl; 
    peer_buffer->stop_buffering();

    // signal parent migration
    auto parent = topo.get_nth_parent(vm, 1); 
    start_gw_migration_if_possible(parent); 

    // one VM migration finished. Start the next one.
    if (vm_migration_queue.size() > 0){
        auto next_vm = vm_migration_queue.front();
        vm_migration_queue.pop();
        start_vm_precopy(next_vm);
    } 
}


void OrchestratorV2::start_gw_migration_if_possible(
                                               Node* gw){
    auto& topo = MyTopology::instance();

    // check if all the children of this node has migrated.
    for(auto& child: topo.get_children(gw)){
        if(mig_state[child] != MigState::Migrated){
            return; 
        }
    }

    print_time(); 
    std::cout << "all conditions ok to start migrating GW: ";
    std::cout << gw->address() << std::endl;

    start_gw_snapshot(gw); 
}


void OrchestratorV2::start_gw_snapshot(Node* gw){
    print_time(); 
    std::cout << "sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

    mig_state[gw] = MigState::PreMig; 

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_snapshot_sent(n);}
    );
}


void OrchestratorV2::gw_snapshot_sent(Node* gw){
    print_time(); 
    std::cout << "finished sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    if (gw == topo.get_mig_root()){
        start_gw_diff(gw);
    } else {
        // mark_last_packet(topo.get_nth_parent(gw, 1), gw);
    }
}; 


void OrchestratorV2::start_gw_diff(Node* gw){
    print_time(); 
    std::cout << "sending the GW diff for GW: ";
    std::cout << gw->address() << std::endl;

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_diff_sent(n);}
    );
}


void OrchestratorV2::gw_diff_sent(Node* gw){
    print_time(); 
    std::cout << "GW migration finished for GW: ";
    std::cout << gw->address() << std::endl;

    mig_state[gw] = MigState::Migrated; 

    auto& topo = MyTopology::instance();
    
    if (gw != topo.get_mig_root()){
        auto parent = topo.get_nth_parent(gw, 1); 
        start_gw_migration_if_possible(parent);
    }
}; 

double OrchestratorV2::random_wait(){
    int r = 80 + std::rand() % 40;
    double wait = (double) r / 100.0; 
    return 0; 
}

