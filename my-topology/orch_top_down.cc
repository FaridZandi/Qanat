#include "orch_top_down.h"
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


OrchTopDown::OrchTopDown() : BaseOrchestrator(){ 
}

OrchTopDown::~OrchTopDown(){

}

void OrchTopDown::dequeue_next_node(){
    while (migration_queue.size() > 0 and 
           in_migration_nodes < MyTopology::parallel_mig){

        auto node = migration_queue.top(); 
        migration_queue.pop();

        in_migration_nodes ++;

        if (is_gateway(node)){
            start_gw_migration(node);
        } else {
            start_vm_precopy(node);
        }
    }
} 


bool OrchTopDown::all_children_migrated(Node* node){
    auto& topo = MyTopology::instance();

    for(auto& child: topo.get_children(node)){
        if(mig_state[child] != MigState::Migrated){
            return false; 
        }
    }

    return true; 
}

void OrchTopDown::end_parent_precopy_if_needed(Node* gw){
    auto& topo = MyTopology::instance();

    auto parent = topo.get_nth_parent(gw, 1);

    if(parent != nullptr){
        if(all_children_migrated(parent)){
            log_event("end gw precopy", parent);
        }
    }
}



void OrchTopDown::tunnel_subtree_tru_parent(Node* node){
    auto& topo = MyTopology::instance();

    if (topo.get_mig_root() == node){
        //nothing  
    } else {
        // setup tunnels for all the nodes in one layer up. 
        int node_layer = topo.get_data(node).layer_from_bottom;
        auto parent = topo.get_nth_parent(node, 1); 

        for(auto leaf: topo.get_leaves(node)){
            // std::cout << "tunneling " << leaf->address() << " through " << parent->address() << std::endl;
            topo.setup_nth_layer_tunnel(leaf, node_layer + 1);
        }
    }
}

void OrchTopDown::start_migration(){
    std::srand(123);

    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.get_mig_root(); 

    for(auto& node: topo.get_all_nodes(mig_root)){    
        set_node_state(node, MigState::Normal);
        set_peer_state(node, MigState::OutOfService);
    }

    migration_queue.push(mig_root);
    dequeue_next_node(); 
};


void OrchTopDown::start_gw_migration(Node* gw){
    auto& topo = MyTopology::instance();

    set_node_state(gw, MigState::InMig);
    set_peer_state(gw, MigState::Buffering);

    for (auto& node: topo.get_children(gw)){
        tunnel_subtree_tru_parent(node);
    } 
    
    if (topo.get_mig_root() == gw){
        topo.sent_traffic_to_dest(); 
    } 

    buffer_on_peer(gw);
    start_gw_snapshot(gw);
}


void OrchTopDown::start_gw_snapshot(Node* gw){

    auto& topo = MyTopology::instance();

    log_event("start gw migration", gw);

    initiate_data_transfer(
        gw, get_random_transfer_size(MyTopology::gw_snapshot_size, 10), 
        [](Node* n){
            auto& orch = OrchTopDown::instance();
            orch.gw_snapshot_send_ack_from_peer(n);
        }
    );
}



void OrchTopDown::gw_snapshot_send_ack_from_peer(Node* gw){
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer of gw sends the S_ack to gw

    log_event("send snapshot ack from", peer);

    initiate_data_transfer(
        peer, 100,
        [](Node* n){
            auto& orch = OrchTopDown::instance();
            orch.gw_snapshot_ack_rcvd(n);
        }
    );    
}

void OrchTopDown::gw_snapshot_ack_rcvd(Node* gw){
    // here Node gw is the gw at the destination
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer is gw at the source
    log_event("snapshot ack recvd in", peer);

    // source node sends the ack 0f S_ack to its peer
    initiate_data_transfer(
        peer, 100, 
        [](Node* n){
            auto& orch = OrchTopDown::instance();
            orch.gw_start_processing_buffer_on_peer(n);
        }
    );   
}

void OrchTopDown::gw_start_processing_buffer_on_peer(Node* gw){
    // here Node gw is the gw at the source
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer is gw at the source
    set_node_state(gw, MigState::Migrated);
    set_peer_state(gw, MigState::Normal);
    process_on_peer(gw);

    log_event("end gw migration", gw);
    log_event("start gw precopy", gw);
 
    end_parent_precopy_if_needed(gw);

    for (auto& node: topo.get_children(gw)){
        migration_queue.push(node);
    } 

    in_migration_nodes --; 
    dequeue_next_node(); 
}; 



  
void OrchTopDown::start_vm_precopy(Node* vm){

    set_node_state(vm, MigState::PreMig);
    set_peer_state(vm, MigState::PreMig);

    log_event("start vm precopy", vm);

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_precopy_size, 10), 
        [](Node* n){
            auto& orch = OrchTopDown::instance();
            orch.vm_precopy_finished(n);
        }
    );
}

void OrchTopDown::vm_precopy_finished(Node* vm){
    log_event("end vm precopy", vm);
    start_vm_migration(vm); 
}; 



void OrchTopDown::start_vm_migration(Node* vm){
    set_node_state(vm, MigState::InMig);
    set_peer_state(vm, MigState::Buffering);

    auto& topo = MyTopology::instance();
    auto parent_gw = topo.get_nth_parent(vm,1);

    buffer_on_peer(vm);
    
    log_event("start vm migration", vm);

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_snapshot_size, 10), 
        [](Node* n){
            auto& orch = OrchTopDown::instance();
            orch.vm_migration_finished(n);
        }
    );
}


void OrchTopDown::vm_migration_finished(Node* vm){

    auto& topo = MyTopology::instance();

    set_node_state(vm, MigState::Migrated);
    set_peer_state(vm, MigState::Normal);

    log_event("end vm migration", vm);
    process_on_peer(vm); 

    in_migration_nodes --;
    dequeue_next_node();

    end_parent_precopy_if_needed(vm);

    bool all_vms_migrated = true; 
    for (auto& node: topo.get_leaves(topo.get_mig_root())){
        if (mig_state[node] != MigState::Migrated){
            all_vms_migrated = false;
            break;
        }
    }

    if (all_vms_migrated){
        log_event("migration finished", vm);
        topo.migration_finished(); 
    }
}



std::list<nf_spec> OrchTopDown::get_vm_nf_list(){
    return {
        {"tunnel_manager", 0}, //Must have

        {"buffer", 100000},
        {"delayer", 0.00005},
        {"monitor", 0},

        {"tunnel_manager", 0}, //Must have
        {"router", 0} //Must have
    };
}

std::list<nf_spec> OrchTopDown::get_gw_nf_list(){
    return {
        {"priority_buffer", 100000},
        {"delayer", 0.00005},
        {"monitor", 0},

        
        {"tunnel_manager", 0}, //Must have
        {"router", 0}, //Must have
    };
}
