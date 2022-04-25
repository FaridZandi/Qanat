#include "orchestrator.h"
#include "orch_v1.h"
#include "orch_v2.h"
#include "my_topology.h"
#include "node.h"

BaseOrchestrator& BaseOrchestrator::instance(){
    // return OrchestratorV1::instance(); 
    return OrchestratorV2::instance(); 
    // return Orchestrator::instance(); 
};


void BaseOrchestrator::initiate_data_transfer(
                            Node* node, int size, 
                            void (*callback) (Node*)){
    auto& topo = MyTopology::instance();

    topo.connect_agents(node, topo.get_peer(node));

    auto tcp_name = topo.get_data(node).tcp.c_str();
    Agent* agent = (Agent*)TclObject::lookup(tcp_name);
    agent->finish_notify_callback = callback; 
    agent->is_finish_callback_set = true;

    topo.send_data(node, size);
}


Orchestrator::Orchestrator() {

}

Orchestrator::~Orchestrator(){

}


