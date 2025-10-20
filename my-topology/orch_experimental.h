#ifndef orch_experimental_h
#define orch_experimental_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>


class Node; 

// Test version of Orchestrator
class OrchExperimental : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchExperimental& instance() {
        static OrchExperimental instance; 
        return instance; 
	}
    
    OrchExperimental(); 
    virtual ~OrchExperimental(); 

    virtual void start_migration();
    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();

private:    
};

#endif
