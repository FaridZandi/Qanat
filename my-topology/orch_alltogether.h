#ifndef orch_alltogether_h
#define orch_alltogether_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>


class Node; 

// Test version of Orchestrator
class OrchAllTogether : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchAllTogether& instance() {
        static OrchAllTogether instance; 
        return instance; 
	}
    
    OrchAllTogether(); 
    virtual ~OrchAllTogether(); 

    virtual void start_migration();
    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();

private:    
};

#endif
