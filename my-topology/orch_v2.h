#ifndef orch_v2_h
#define orch_v2_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>


class Node; 

// Test version of Orchestrator
class OrchestratorV2 : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchestratorV2& instance() {
        static OrchestratorV2 instance; 
        return instance; 
	}
    
    OrchestratorV2(); 
    virtual ~OrchestratorV2(); 

    virtual void start_background_traffic(){};
    virtual void setup_nodes(); 
    virtual void start_migration();

    virtual void vm_precopy_finished(Node* vm); 
    virtual void vm_migration_finished(Node* vm); 
    virtual void gw_snapshot_sent(Node* gw); 
    virtual void gw_diff_sent(Node* gw); 
    virtual void gw_sent_last_packet(Node* gw){}; 
    virtual void gw_received_last_packet(Node* gw){}; 

private: 

    void start_vm_precopy(Node* vm); 
    void start_vm_migration(Node* vm); 
    void start_gw_snapshot(Node* gw); 
    void start_gw_diff(Node* gw);  
    void copy_gw_state(Node* gw){}; 
    void start_gw_migration_if_possible(Node* gw);     

    // utility
    
    double random_wait(); 

    // data 

    std::queue<Node*> vm_migration_queue;
    std::map<Node*, MigState> mig_state; 

    static const int parallel_migrations = 100; 
};


#endif

