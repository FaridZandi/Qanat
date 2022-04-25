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
#include "nf.h"
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include "utility.h"


OrchestratorV1::OrchestratorV1() {

}

OrchestratorV1::~OrchestratorV1(){

}



void OrchestratorV1::setup_nodes(){
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
        if(vm_migration_queue.size() > 0){

            auto node = vm_migration_queue.front(); 
            vm_migration_queue.pop();
            start_vm_precopy(node);
        }
    }
};
    
  
void OrchestratorV1::start_vm_precopy(Node* vm){
    print_time(); 
    std::cout << "sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    mig_state[vm] = MigState::PreMig; 

    initiate_data_transfer(
        vm, 100000000, 
        [](Node* n){BaseOrchestrator::instance().vm_precopy_finished(n);}
    );
}

void OrchestratorV1::vm_precopy_finished(Node* vm){
    print_time(); 
    std::cout << "finished sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    start_vm_migration(vm); 
}; 


void OrchestratorV1::start_vm_migration(Node* vm){
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


void OrchestratorV1::vm_migration_finished(Node* vm){
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


void OrchestratorV1::start_gw_migration_if_possible(
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


void OrchestratorV1::start_gw_snapshot(Node* gw){
    print_time(); 
    std::cout << "sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

    mig_state[gw] = MigState::PreMig; 

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_snapshot_sent(n);}
    );
}


void OrchestratorV1::gw_snapshot_sent(Node* gw){
    print_time(); 
    std::cout << "finished sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

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
    
    print_time(); 
    std::cout << "last packet has been sent to GW: "; 
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    // setup tunnels for all the nodes in one layer up. 
    int gw_layer = topo.get_data(gw).layer_from_bottom;
    auto parent = topo.get_nth_parent(gw, 1); 

    for(auto vm: topo.get_leaves(gw)){
        topo.setup_nth_layer_tunnel(vm, gw_layer + 1);
    }

    auto lpr = new LastPacketReceiver; 
    lpr->gw = gw; 
    lpr->sched(random_wait()); 
}


void OrchestratorV1::gw_received_last_packet(Node* gw){
    print_time(); 
    std::cout << "last packet has been received by GW: "; 
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    mig_state[gw] = MigState::InMig; 

    start_gw_diff(gw); 
}


void OrchestratorV1::start_gw_diff(Node* gw){
    print_time(); 
    std::cout << "sending the GW diff for GW: ";
    std::cout << gw->address() << std::endl;

    initiate_data_transfer(
        gw, 1000000, 
        [](Node* n){BaseOrchestrator::instance().gw_diff_sent(n);}
    );
}


void OrchestratorV1::gw_diff_sent(Node* gw){
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

double OrchestratorV1::random_wait(){
    int r = 80 + std::rand() % 40;
    double wait = (double) r / 100.0; 
    return 0; 
}

void LastPacketSender::expire(Event* e){  
    auto& orch = OrchestratorV1::instance(); 
    orch.gw_sent_last_packet(gw);  
}

void LastPacketReceiver::expire(Event* e){  
    auto& orch = OrchestratorV1::instance(); 
    orch.gw_received_last_packet(gw);  
}

