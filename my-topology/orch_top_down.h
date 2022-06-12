#ifndef orch_top_down_h
#define orch_top_down_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>

class Node; 

// Test version of Orchestrator
class OrchTopDown : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchTopDown& instance() {
        static OrchTopDown instance; 
        return instance; 
	}
    
    OrchTopDown(); 
    virtual ~OrchTopDown(); 

    virtual void start_migration();

    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();

private: 
    
};


#endif

