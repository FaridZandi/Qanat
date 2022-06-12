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


void OrchTopDown::start_migration(){
    std::srand(123);

    auto& topo = MyTopology::instance(); 
    auto mig_root = topo.get_mig_root(); 
};




std::list<nf_spec> OrchTopDown::get_vm_nf_list(){
    return {
        {"buffer", 2400},
        {"rate_limiter", 1000000},
        {"delayer", 0.000005},

        //Must have these two NFs at the end of the list
        {"tunnel_manager", 0},
        {"router", 0}
    };
}

std::list<nf_spec> OrchTopDown::get_gw_nf_list(){
    return {
        {"priority_buffer", 2400},
        {"delayer", 0.00005},
        {"monitor", 0},

        //Must have these two NFs at the end of the list
        {"tunnel_manager", 0},
        {"router", 0}
    };
}
