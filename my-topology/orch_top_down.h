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

    void gw_snapshot_send_ack_from_peer(Node* gw);
    void gw_start_processing_buffer_on_peer(Node* gw);
    void gw_snapshot_ack_rcvd(Node* gw); 
    void vm_precopy_finished(Node* vm); 
    void vm_migration_finished(Node* vm); 

    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();

private: 
    void tunnel_subtree_tru_parent(Node* node);
    bool all_children_migrated(Node* node); 
    void start_vm_precopy(Node* vm); 
    void start_vm_migration(Node* vm); 
    void start_gw_migration(Node* gw);
    void start_gw_snapshot(Node* gw);
    void end_parent_precopy_if_needed(Node* gw);
    void dequeue_next_node();
};


#endif

