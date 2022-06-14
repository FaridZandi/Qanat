#include "orch_bottom_up.h"
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


OrchBottomUp::OrchBottomUp() : BaseOrchestrator(){    
}

OrchBottomUp::~OrchBottomUp(){

}

void OrchBottomUp::tunnel_subtree_tru_parent(Node* node){
    auto& topo = MyTopology::instance();

    if (topo.get_mig_root() == node){
        topo.sent_traffic_to_dest(); 
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
            log_event("start gw precopy", topo.get_peer(parent));
        }
    }
}

void OrchBottomUp::start_migration(){
    std::srand(123);

    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.get_mig_root(); 

    std::cout << "start migration" << std::endl;

    std::cout << "VM migration Queue: ";
    for (auto& leaf: topo.get_leaves(mig_root)){
        vm_migration_queue.push(leaf);
        std::cout << leaf->address() << " ";
    }
    std::cout << std::endl;

    for(auto& node: topo.get_all_nodes(mig_root)){    
        set_node_state(node, MigState::Normal);
        set_peer_state(node, MigState::OutOfService);
    }

    int parallel_migrations = MyTopology::parallel_mig; 
    std::cout << "Parallel migrations: " << parallel_migrations << std::endl;
    for(int i = 0; i < parallel_migrations; i++){
        dequeue_next_vm();
    }
};

  
void OrchBottomUp::start_vm_precopy(Node* vm){
    set_node_state(vm, MigState::PreMig);
    set_peer_state(vm, MigState::OutOfService);
    log_event("start vm precopy", vm);

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_precopy_size, 10), 
        [](Node* n){
            auto& orch = OrchBottomUp::instance();
            orch.vm_precopy_finished(n);
        }
    );
}

void OrchBottomUp::vm_precopy_finished(Node* vm){
    log_event("end vm precopy", vm);
    start_vm_migration(vm); 
}; 



void OrchBottomUp::start_vm_migration(Node* vm){
    set_node_state(vm, MigState::InMig);
    set_peer_state(vm, MigState::Buffering);

    MyTopology::instance().setup_nth_layer_tunnel(vm, 1);
    
    auto& topo = MyTopology::instance();
    auto parent_gw = topo.get_nth_parent(vm,1);

    if (mig_state[parent_gw] == MigState::Normal){
        set_node_state(parent_gw, MigState::PreMig);
        set_peer_state(parent_gw, MigState::PreMig);
        log_event("start gw precopy", parent_gw);
        log_event("start gw precopy", topo.get_peer(parent_gw));
    }

    buffer_on_peer(vm);

    log_event("start vm migration", vm);

    // std::cout << "MyTopology::vm_snapshot_size: " << MyTopology::vm_snapshot_size << std::endl; 

    initiate_data_transfer(
        vm, get_random_transfer_size(MyTopology::vm_snapshot_size, 10), 
        [](Node* n){
            auto& orch = OrchBottomUp::instance();
            orch.vm_migration_finished(n);
        }
    );
}


void OrchBottomUp::vm_migration_finished(Node* vm){
    set_node_state(vm, MigState::Migrated);
    set_peer_state(vm, MigState::Normal);
    log_event("end vm migration", vm);
    process_on_peer(vm); 
    try_parent_migration(vm); 
    dequeue_next_vm();
}


void OrchBottomUp::try_parent_migration(Node* node){
    auto& topo = MyTopology::instance();
    auto gw = topo.get_nth_parent(node, 1); 

    if (all_children_migrated(gw)){
        // log_event("all conditions ok to start migrating GW: ", gw->address()); 
        buffer_on_peer(gw);
        set_peer_state(gw, MigState::Buffering);
        start_gw_snapshot(gw); 
    }
}


void OrchBottomUp::start_gw_snapshot(Node* gw){
    auto& topo = MyTopology::instance();
    set_node_state(gw, MigState::InMig);
    
    log_event("end gw precopy", gw);
    log_event("end gw precopy", topo.get_peer(gw));

    log_event("start gw migration", gw);

    initiate_data_transfer(
        gw, get_random_transfer_size(MyTopology::gw_snapshot_size, 10), 
        [](Node* n){
            auto& orch = OrchBottomUp::instance();
            orch.gw_snapshot_send_ack_from_peer(n);
        }
    );
}


void OrchBottomUp::gw_snapshot_send_ack_from_peer(Node* gw){
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer of gw sends the S_ack to gw

    log_event("send snapshot ack from", peer);

    initiate_data_transfer(
        peer, 100,
        [](Node* n){
            auto& orch = OrchBottomUp::instance();
            orch.gw_snapshot_ack_rcvd(n);
        }
    );    
}

void OrchBottomUp::gw_snapshot_ack_rcvd(Node* gw){
    // here Node gw is the gw at the destination
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer is gw at the source
    log_event("snapshot ack recvd in", peer);

    // source node sends the ack 0f S_ack to its peer
    initiate_data_transfer(
        peer, 100, 
        [](Node* n){
            auto& orch = OrchBottomUp::instance();
            orch.gw_start_processing_buffer_on_peer(n);
        }
    ); 
    // start to send the data through the lower priority tunnel
    tunnel_subtree_tru_parent(peer);   
}

void OrchBottomUp::gw_start_processing_buffer_on_peer(Node* gw){
    // here Node gw is the gw at the source
    auto& topo = MyTopology::instance();   
    auto peer = topo.get_peer(gw); // peer is gw at the source

    log_event("end gw migration", gw);
 
    process_on_peer(gw);

    set_node_state(gw, MigState::Migrated);
    set_peer_state(gw, MigState::Normal);

    if (gw == topo.get_mig_root()){
        log_event("migration finished", gw);
        migration_finished();
    } else {
        try_parent_migration(gw); 
    }
}; 


void OrchBottomUp::dequeue_next_vm(){
    if(vm_migration_queue.size() > 0){
        auto node = vm_migration_queue.front(); 
        vm_migration_queue.pop();
        start_vm_precopy(node);
    }
} 

bool OrchBottomUp::all_children_migrated(Node* node){
    auto& topo = MyTopology::instance();

    for(auto& child: topo.get_children(node)){
        if(mig_state[child] != MigState::Migrated){
            return false; 
        }
    }

    return true; 
}

void OrchBottomUp::migration_finished(){
    auto& topo = MyTopology::instance();   
    topo.migration_finished(); 
}


std::list<nf_spec> OrchBottomUp::get_vm_nf_list(){
    return {
        {"buffer", 2400},
        {"delayer", 0.00005},

        //Must have these two NFs at the end of the list
        {"tunnel_manager", 0},
        {"router", 0}
    };
}

std::list<nf_spec> OrchBottomUp::get_gw_nf_list(){
    return {
        {"priority_buffer", 2400},
        {"delayer", 0.00005},

        // gateways have monitoring NFs
        {"monitor", 0},

        //Must have these two NFs at the end of the list
        {"tunnel_manager", 0},
        {"router", 0}
    };
}
