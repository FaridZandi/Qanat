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
    virtual void start_migration();

    virtual void vm_precopy_finished(Node* vm); 
    virtual void vm_migration_finished(Node* vm); 
    virtual void gw_snapshot_sent(Node* gw); 
    virtual void gw_sent_last_packet(Node* gw){}; 
    virtual void gw_received_last_packet(Node* gw){}; 

    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();

    


private: 

    void dequeue_next_vm();  
    bool all_children_migrated(Node* node); 

    void start_vm_precopy(Node* vm); 
    void start_vm_migration(Node* vm); 
    void start_gw_snapshot(Node* gw); 
    void copy_gw_state(Node* gw){}; 
    void try_parent_migration(Node* gw);

    // std::map<Node*, bool> gw_snap_shot_sent; 
    // std::map<Node*, bool> gw_peer_timeout; 

    static const int VM_PRECOPY_SIZE = 10000000; 
    static const int VM_SNAPSHOT_SIZE = 10000000;      
    static const int GW_SNAPSHOT_SIZE = 10000000;      
};


#endif

