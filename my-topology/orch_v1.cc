#include "orch_v1.h"
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


OrchestratorV1::OrchestratorV1() {

}

OrchestratorV1::~OrchestratorV1(){

}


void OrchestratorV1::start_migration(){
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
        if (vm_migration_queue.size() > 0){
            auto next_vm = vm_migration_queue.front();
            vm_migration_queue.pop();
            start_vm_precopy(next_vm);
        } 
    }
};
    
  
void OrchestratorV1::start_vm_precopy(Node* vm){
    log_event("sending the VM precopy for node: ", vm->address());

    mig_state[vm] = MigState::PreMig; 

    initiate_data_transfer(
        vm, 100000000, 
        [](Node* n){BaseOrchestrator::instance().vm_precopy_finished(n);}
    );
}

void OrchestratorV1::vm_precopy_finished(Node* vm){
    log_event("finished sending the VM precopy for node: ", vm->address());

    start_vm_migration(vm); 
}; 


void OrchestratorV1::start_vm_migration(Node* vm){
    mig_state[vm] = MigState::InMig; 

    auto& topo = MyTopology::instance();
    auto peer = topo.get_peer(vm);
    auto peer_data = topo.get_data(peer);
    auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");
    
    topo.setup_nth_layer_tunnel(vm, 1); 

    log_event("start buffering for ", peer->address());

    peer_buffer->start_buffering();

    log_event("sending the VM snapshot for node: ", vm->address());

    initiate_data_transfer(
        vm, 10000000, 
        [](Node* n){BaseOrchestrator::instance().vm_migration_finished(n);}
    );
}


void OrchestratorV1::vm_migration_finished(Node* vm){
    log_event("VM migration finished for: ", vm->address());

    mig_state[vm] = MigState::Migrated; 

    auto& topo = MyTopology::instance();

    auto peer = topo.get_peer(vm);
    auto peer_data = topo.get_data(peer);
    auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");

    log_event("stopped buffering for ", peer->address());
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


void OrchestratorV1::start_gw_migration_if_possible(
                                               Node* gw){
    auto& topo = MyTopology::instance();

    // check if all the children of this node has migrated.
    for(auto& child: topo.get_children(gw)){
        if(mig_state[child] != MigState::Migrated){
            return; 
        }
    }

    log_event("all conditions ok to start migrating GW: ", gw->address()); 

    start_gw_snapshot(gw); 
}


void OrchestratorV1::start_gw_snapshot(Node* gw){
    log_event("sending the GW snapshot for GW: ", gw->address());

    mig_state[gw] = MigState::PreMig; 

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_snapshot_sent(n);}
    );
}


void OrchestratorV1::gw_snapshot_sent(Node* gw){
    log_event("finished sending the GW snapshot for GW: ", gw->address());

    auto& topo = MyTopology::instance();

    if (gw == topo.get_mig_root()){
        start_gw_diff(gw);
    } else {
        mark_last_packet(topo.get_nth_parent(gw, 1), gw);
    }
}; 



void OrchestratorV1::mark_last_packet(Node* parent, 
                                          Node* child){
                                            
    print_time(); 
    std::cout << "waiting of the last packet to be sent "; 
    std::cout << "from GW: " << parent->address() << " "; 
    std::cout << "to GW: " << child->address() << std::endl;


    auto lps = new LastPacketSender; 
    lps->gw = child;
    // This doesn't matter right now, but ultimately 
    // this is how the parent node would know that it has 
    // sent the last packet to 
    // auto children = topo.get_leaves(gw);
    lps->sched(random_wait()); 
}

void OrchestratorV1::gw_sent_last_packet(Node* gw){
    
    log_event("last packet has been sent to GW: ", gw->address());

    tunnel_subtree_tru_parent(gw);

    auto lpr = new LastPacketReceiver; 
    lpr->gw = gw; 
    lpr->sched(random_wait()); 
}


void OrchestratorV1::gw_received_last_packet(Node* gw){
    log_event("last packet has been received by GW: ", gw->address());

    auto& topo = MyTopology::instance();

    mig_state[gw] = MigState::InMig; 

    start_gw_diff(gw); 
}


void OrchestratorV1::start_gw_diff(Node* gw){
    log_event("sending the GW diff for GW: ", gw->address());

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_diff_sent(n);}
    );
}


void OrchestratorV1::gw_diff_sent(Node* gw){
    log_event("GW migration finished for GW: ", gw->address());

    mig_state[gw] = MigState::Migrated; 

    auto& topo = MyTopology::instance();
    
    if (gw != topo.get_mig_root()){
        auto parent = topo.get_nth_parent(gw, 1); 
        start_gw_migration_if_possible(parent);
    }
}; 

void LastPacketSender::expire(Event* e){  
    auto& orch = OrchestratorV1::instance(); 
    orch.gw_sent_last_packet(gw);  
}

void LastPacketReceiver::expire(Event* e){  
    auto& orch = OrchestratorV1::instance(); 
    orch.gw_received_last_packet(gw);  
}

std::list<nf_spec> OrchestratorV1::get_vm_nf_list(){
    return {
        {"buffer", 100},
        {"rate_limiter", 100},
        {"delayer", 0.01},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}

std::list<nf_spec> OrchestratorV1::get_gw_nf_list(){
    return {
        {"buffer", 100},
        {"rate_limiter", 100},
        {"monitor", 0},
        {"delayer", 0.01},
        {"tunnel_manager", 0},
        {"router", 0}
    };
}
