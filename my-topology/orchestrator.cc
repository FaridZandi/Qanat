#include "orchestrator.h"
#include "orch_stupid.h"
#include "my_topology.h"
#include "node.h"

BaseOrchestrator& BaseOrchestrator::instance(){
    return StupidOrchestrator::instance(); 
    // return Orchestrator::instance(); 
};

Orchestrator::Orchestrator() {

}

Orchestrator::~Orchestrator(){

}
