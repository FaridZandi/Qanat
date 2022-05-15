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


int get_random_transfer_size(int mean, int range_p){
    int r = (100 - range_p) + std::rand() % (2 * range_p);
    int size = r * mean / 100;
    return size; 
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

    log_event("start vm precopy", vm);

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_precopy_size,50), 
        [](Node* n){
            BaseOrchestrator::instance().vm_precopy_finished(n);
        }
    );
}

void OrchestratorV2::vm_precopy_finished(Node* vm){
    log_event("end vm precopy", vm);

    start_vm_migration(vm); 
}; 



void OrchestratorV2::start_vm_migration(Node* vm){
    set_node_state(vm, MigState::InMig);
    set_peer_state(vm, MigState::Buffering);

    MyTopology::instance().setup_nth_layer_tunnel(vm, 1);
    
    auto& topo = MyTopology::instance();
    auto parent_gw = topo.get_nth_parent(vm,1);
    if (mig_state[parent_gw] == MigState::Normal){
        set_node_state(parent_gw, MigState::PreMig);
        set_peer_state(parent_gw, MigState::PreMig);
        log_event("start gw precopy", parent_gw);
    }

    buffer_on_peer(vm);
    
    log_event("start vm migration", vm);

    std::cout << "MyTopology::vm_snapshot_size: " << MyTopology::vm_snapshot_size << std::endl; 

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_snapshot_size, 50), 
        [](Node* n){
            BaseOrchestrator::instance().vm_migration_finished(n);
        }
    );
}


void OrchestratorV2::vm_migration_finished(Node* vm){
    set_node_state(vm, MigState::Migrated);
    set_peer_state(vm, MigState::Normal);

    log_event("end vm migration", vm);

    process_on_peer(vm); 
    
    try_parent_migration(vm); 

    dequeue_next_vm();
}


void OrchestratorV2::try_parent_migration(Node* node){
    auto& topo = MyTopology::instance();
    auto gw = topo.get_nth_parent(node, 1); 

    if (all_children_migrated(gw)){
        tunnel_subtree_tru_parent(gw);
        // log_event("all conditions ok to start migrating GW: ", gw->address()); 

        buffer_on_peer(gw);
        set_peer_state(gw, MigState::Buffering);

        start_gw_snapshot(gw); 
    }
}


void OrchestratorV2::start_gw_snapshot(Node* gw){
    set_node_state(gw, MigState::InMig);
    
    log_event("end gw precopy", gw);

    log_event("start gw migration", gw);

    initiate_data_transfer(
        gw, get_random_transfer_size(MyTopology::gw_snapshot_size, 50), 
        [](Node* n){
            BaseOrchestrator::instance().gw_snapshot_sent(n);
        }
    );
}


void OrchestratorV2::gw_snapshot_sent(Node* gw){

    log_event("end gw migration", gw);

    auto& topo = MyTopology::instance();   
    
    process_on_peer(gw);

    set_node_state(gw, MigState::Migrated);
    set_peer_state(gw, MigState::Normal);

    if (gw == topo.get_mig_root()){
        log_event("migration finished", gw);
        exit(0);
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
