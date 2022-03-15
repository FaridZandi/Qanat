#include "orch_stupid.h"
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


StupidOrchestrator::StupidOrchestrator() {

}

StupidOrchestrator::~StupidOrchestrator(){

}

void StupidOrchestrator::setup_node_types(){
    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.mig_root;
    auto mig_root_peer = topo.data[mig_root].peer;


    for(auto& node: topo.get_leaves(mig_root)){
        mig_state[node] = MigState::NoMigState;
    }

    for(auto& node: topo.get_internal_nodes(mig_root)){
        mig_state[node] = MigState::NoMigState;
    }

    // TODO: I should probably move this to another class
    // This both does and does not seem to be right place 
    // for these stuff. 

    for (auto& root: std::list<Node*>({mig_root, mig_root_peer})){
        for(auto& node: topo.get_leaves(root)){        
            topo.data[node].mode = OpMode::VM;
            topo.data[node].add_nf("buffer", 100);
            topo.data[node].add_nf("rate_limiter", 100);
            topo.data[node].add_nf("delayer", 0.01);
            topo.data[node].add_nf("tunnel_manager");
            topo.data[node].print_nfs(); 
        }
        for(auto& node: topo.get_internal_nodes(root)){        
            topo.data[node].mode = OpMode::GW;
            auto m = (Monitor*)topo.data[node].add_nf("monitor");

            ////////////////////////////////
            // if (root == mig_root){
            //     m->is_recording = true; 
            // } else {
            //     m->is_loading = true; 
            // }
            ////////////////////////////////
            
            topo.data[node].add_nf("delayer", 0.01);
            topo.data[node].add_nf("tunnel_manager");
            topo.data[node].print_nfs(); 
        }
    }

    std::cout << "--------------------------" << std::endl;
    std::cout << "--------------------------" << std::endl;
    std::cout << "--------------------------" << std::endl;

    topo.introduce_nodes_to_classifiers(); 
};

void StupidOrchestrator::start_migration(){
    std::srand(123);

    auto& topo = MyTopology::instance(); 

    std::cout << "VM migration Queue: ";
    for (auto& leave : topo.get_leaves(topo.mig_root)){
        vm_migration_queue.push(leave);
        std::cout << leave->address() << " ";
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
    
  
void StupidOrchestrator::start_vm_precopy(Node* vm){
    print_time(); 
    std::cout << "sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    mig_state[vm] = MigState::PreMig; 

    auto t = new VMPrecopySender; 
    t->vm = vm; 
    t->sched(random_wait()); 
}

void StupidOrchestrator::vm_precopy_finished(Node* vm){
    print_time(); 
    std::cout << "finished sending the VM precopy for node: ";
    std::cout << vm->address() << std::endl;

    start_vm_migration(vm); 
}; 


void StupidOrchestrator::start_vm_migration(Node* vm){
    mig_state[vm] = MigState::InMig; 

    auto& topo = MyTopology::instance();
    auto peer = topo.data[vm].peer;
    auto peer_data = topo.data[peer];
    auto peer_buffer = (Buffer*)peer_data.get_nf("buffer");
    
    setup_nth_layer_tunnel(vm, 1); 

    print_time(); 
    std::cout << "start buffering for ";
    std::cout << peer->address() << std::endl; 
    peer_buffer->start_buffering();

    print_time(); 
    std::cout << "sending the VM snanshot for node: ";
    std::cout << vm->address() << std::endl;

    auto vmss = new VMSnapshotSender; 
    vmss->vm = vm; 
    vmss->sched(random_wait());  
}



void StupidOrchestrator::vm_migration_finished(Node* vm){
    print_time(); 
    std::cout << "VM migration finished for: ";
    std::cout << vm->address() << std::endl;
    
    mig_state[vm] = MigState::Migrated; 

    auto& topo = MyTopology::instance();

    auto peer = topo.data[vm].peer;
    auto peer_data = topo.data[peer];
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


void StupidOrchestrator::start_gw_migration_if_possible(
                                               Node* gw){
    auto& topo = MyTopology::instance();

    // check if all the children of this node has migrated.
    for(auto& child: topo.data[gw].children){
        if(mig_state[child] != MigState::Migrated){
            return; 
        }
    }

    print_time(); 
    std::cout << "all conditions ok to start migrating GW: ";
    std::cout << gw->address() << std::endl;

    start_gw_snapshot(gw); 
}


void StupidOrchestrator::start_gw_snapshot(Node* gw){
    print_time(); 
    std::cout << "sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

    mig_state[gw] = MigState::PreMig; 

    auto gwss = new GWSnapshotSender;
    gwss->gw = gw;
    gwss->sched(random_wait());
}


void StupidOrchestrator::gw_snapshot_sent(Node* gw){
    print_time(); 
    std::cout << "finished sending the GW snapshot for GW: ";
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    if (gw == topo.mig_root){
        start_gw_diff(gw);
    } else {
        mark_last_packet(topo.get_nth_parent(gw, 1), gw);
    }
    
}; 



void StupidOrchestrator::mark_last_packet(Node* parent, 
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

void StupidOrchestrator::gw_sent_last_packet(Node* gw){
    
    print_time(); 
    std::cout << "last packet has been sent to GW: "; 
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    // setup tunnels for all the nodes in one layer up. 
    int gw_layer = topo.data[gw].layer_from_bottom;
    auto parent = topo.data[gw].first_parent(); 

    for(auto vm: topo.get_leaves(gw)){
        setup_nth_layer_tunnel(vm, gw_layer + 1);
    }

    auto lpr = new LastPacketReceiver; 
    lpr->gw = gw; 
    lpr->sched(random_wait()); 
}


void StupidOrchestrator::gw_received_last_packet(Node* gw){
    print_time(); 
    std::cout << "last packet has been received by GW: "; 
    std::cout << gw->address() << std::endl;

    auto& topo = MyTopology::instance();

    mig_state[gw] = MigState::InMig; 

    start_gw_diff(gw); 
}


void StupidOrchestrator::start_gw_diff(Node* gw){
    print_time(); 
    std::cout << "sending the GW diff for GW: ";
    std::cout << gw->address() << std::endl;

    auto gwds = new GWDiffSender;
    gwds->gw = gw;
    gwds->sched(random_wait());
}


void StupidOrchestrator::gw_diff_sent(Node* gw){
    print_time(); 
    std::cout << "GW migration finished for GW: ";
    std::cout << gw->address() << std::endl;

    mig_state[gw] = MigState::Migrated; 

    auto& topo = MyTopology::instance();
    auto parent = topo.get_nth_parent(gw, 1); 

    if (gw != topo.mig_root){
        start_gw_migration_if_possible(parent);
    }
    
}; 


  





void StupidOrchestrator::setup_nth_layer_tunnel(Node* node, int layer){
    auto& topo = MyTopology::instance(); 

    auto peer = topo.data[node].peer; 

    auto nth_parent = topo.get_nth_parent(node, layer); 
    auto nth_parent_peer = topo.data[nth_parent].peer; 

    print_time(); 
    std::cout << "node " << node->address() << " ";                           
    std::cout << "migrated to " << peer->address() << " ";                           
    std::cout << "tunnelled from " << nth_parent->address() << " ";
    std::cout << "to " << nth_parent_peer->address() << " ";
    std::cout << std::endl;

    topo.mig_manager().activate_tunnel(
        nth_parent, nth_parent_peer,  
        node, peer
    );
}; 


void StupidOrchestrator::print_time(){
    std::cout << "[";
    std::cout << std::setprecision(5);
    std::cout << std::setw(5);
    std::cout << Scheduler::instance().clock();
    std::cout << "] ";
}

double StupidOrchestrator::random_wait(){
    int r = 80 + std::rand() % 40;
    double wait = (double) r / 100.0; 
    return wait; 
}









void GWSnapshotSender::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.gw_snapshot_sent(gw);  
}

void GWDiffSender::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.gw_diff_sent(gw);  
}

void VMSnapshotSender::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.vm_migration_finished(vm);  
}

void VMPrecopySender::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.vm_precopy_finished(vm);  
}

void LastPacketSender::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.gw_sent_last_packet(gw);  
}

void LastPacketReceiver::expire(Event* e){  
    auto& orch = StupidOrchestrator::instance(); 
    orch.gw_received_last_packet(gw);  
}

