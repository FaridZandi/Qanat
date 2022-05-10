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
#include "network_func.h"
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include "utility.h"


OrchestratorV2::OrchestratorV2() {

}

OrchestratorV2::~OrchestratorV2(){

}

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
        dequeue_next_vm();
    }
};



  
void OrchestratorV2::start_vm_precopy(Node* vm){

    set_node_state(vm, MigState::PreMig);
    set_peer_state(vm, MigState::OutOfService);

    log_event("sending the VM precopy for node: ", vm->address());

    initiate_data_transfer(
        vm, VM_PRECOPY_SIZE, 
        [](Node* n){
            BaseOrchestrator::instance().vm_precopy_finished(n);
        }
    );
}

void OrchestratorV2::vm_precopy_finished(Node* vm){
    log_event("finished sending the VM precopy for node: ", vm->address());

    start_vm_migration(vm); 
}; 



void OrchestratorV2::start_vm_migration(Node* vm){
    set_node_state(vm, MigState::InMig);
    set_peer_state(vm, MigState::Buffering);

    MyTopology::instance().setup_nth_layer_tunnel(vm, 1); 

    buffer_on_peer(vm);

    log_event("sending the VM snapshot for node: ", vm->address());
    initiate_data_transfer(
        vm, VM_SNAPSHOT_SIZE, 
        [](Node* n){
            BaseOrchestrator::instance().vm_migration_finished(n);
        }
    );
}


void OrchestratorV2::vm_migration_finished(Node* vm){
    set_node_state(vm, MigState::Migrated);
    set_peer_state(vm, MigState::Normal);

    log_event("VM migration finished for: ", vm->address());

    process_on_peer(vm); 
    
    try_parent_migration(vm); 

    dequeue_next_vm();
}


void OrchestratorV2::try_parent_migration(Node* node){
    auto& topo = MyTopology::instance();
    auto gw = topo.get_nth_parent(node, 1); 

    if (all_children_migrated(gw)){

        log_event("all conditions ok to start migrating GW: ", gw->address()); 

        if (gw != topo.get_mig_root()){
            tunnel_subtree_tru_parent(gw);   

            auto peer = topo.get_peer(gw);
            auto peer_data = topo.get_data(peer);
            auto peer_buffer = (SelectiveBuffer*)peer_data.get_nf("selbuf");
            auto parent = topo.get_nth_parent(peer, 1)->address();
            peer_buffer->buffer_packets_from = parent; 
            peer_buffer->start_buffering();
            set_peer_state(gw, MigState::Buffering);
            log_event("starting a selective buffer for ", peer->address());
        }


        start_gw_snapshot(gw); 

    }
}


void OrchestratorV2::start_gw_snapshot(Node* gw){
    set_node_state(gw, MigState::InMig);
    
    log_event("sending the GW snapshot for GW: ", gw->address());

    initiate_data_transfer(
        gw, VM_SNAPSHOT_SIZE, 
        [](Node* n){
            BaseOrchestrator::instance().gw_snapshot_sent(n);
        }
    );
}


void OrchestratorV2::gw_snapshot_sent(Node* gw){

    log_event("finished sending the GW snapshot for GW: ", gw->address());

    auto& topo = MyTopology::instance();

    auto peer = topo.get_peer(gw);
    auto peer_data = topo.get_data(peer);
    auto peer_buffer = (SelectiveBuffer*)peer_data.get_nf("selbuf");

    auto queue_depth_high = peer_buffer->get_buffer_size_highprio();
    auto queue_depth_low = peer_buffer->get_buffer_size_lowprio();
    peer_buffer->stop_buffering();

    set_node_state(gw, MigState::Migrated);
    set_peer_state(gw, MigState::Normal);

    log_event("releasing a high prio buffer of size ", queue_depth_high);
    log_event("releasing a low prio buffer of size ", queue_depth_low);

    if (gw == topo.get_mig_root()){
        log_event("migration finished");
    } else {
        try_parent_migration(gw); 
    }
}; 


void OrchestratorV2::dequeue_next_vm(){
    if(vm_migration_queue.size() > 0){
        auto node = vm_migration_queue.front(); 
        vm_migration_queue.pop();
        start_vm_precopy(node);
    }
} 

bool OrchestratorV2::all_children_migrated(Node* node){
    auto& topo = MyTopology::instance();

    for(auto& child: topo.get_children(node)){
        if(mig_state[child] != MigState::Migrated){
            return false; 
        }
    }
    return true; 
}



std::list<nf_spec> OrchestratorV2::get_vm_nf_list(){
    return {
        {"buffer", 100000},
        {"rate_limiter", 100000},
        {"delayer", 0.00005},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}

std::list<nf_spec> OrchestratorV2::get_gw_nf_list(){
    return {
        {"selective_buffer", 100000},
        {"rate_limiter", 100000},
        {"monitor", 0},
        {"delayer", 0.00005},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}
