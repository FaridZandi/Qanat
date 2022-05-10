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


OrchestratorV2::OrchestratorV2() : BaseOrchestrator(){    
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

    int parallel_migrations = MyTopology::parallel_mig; 
    for(int i = 0; i < parallel_migrations; i++){
        dequeue_next_vm();
    }
};



  
void OrchestratorV2::start_vm_precopy(Node* vm){

    set_node_state(vm, MigState::PreMig);
    set_peer_state(vm, MigState::OutOfService);

    log_event("sending the VM precopy for node: ", vm->address());

    initiate_data_transfer(
        vm, MyTopology::vm_precopy_size, 
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
        vm, MyTopology::vm_snapshot_size, 
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

        buffer_on_peer(gw);
        set_peer_state(gw, MigState::Buffering);

        start_gw_snapshot(gw); 
    }
}


void OrchestratorV2::start_gw_snapshot(Node* gw){
    set_node_state(gw, MigState::InMig);
    
    log_event("sending the GW snapshot for GW: ", gw->address());

    initiate_data_transfer(
        gw, MyTopology::gw_snapshot_size, 
        [](Node* n){
            BaseOrchestrator::instance().gw_snapshot_sent(n);
        }
    );
}


void OrchestratorV2::gw_snapshot_sent(Node* gw){

    log_event("finished sending the GW snapshot for GW: ", gw->address());

    auto& topo = MyTopology::instance();

    tunnel_subtree_tru_parent(gw);   
    
    process_on_peer(gw);

    set_node_state(gw, MigState::Migrated);
    set_peer_state(gw, MigState::Normal);

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
        {"rate_limiter", 1000000},
        {"delayer", 0.000005},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}

std::list<nf_spec> OrchestratorV2::get_gw_nf_list(){
    return {
        {"priority_buffer", 100000},
        {"rate_limiter", 1000000},
        {"monitor", 0},
        {"delayer", 0.000005},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}
