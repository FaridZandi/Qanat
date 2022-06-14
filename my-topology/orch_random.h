#ifndef orch_random_h
#define orch_random_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>

class Node; 

// Test version of Orchestrator
class OrchRandom : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchRandom& instance() {
        static OrchRandom instance; 
        return instance; 
	}
    
    OrchRandom(); 
    virtual ~OrchRandom(); 

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
    void dequeue_next_node();
    bool all_children_migrated(Node* node); 
    void start_vm_precopy(Node* vm); 
    void start_vm_migration(Node* vm); 
    void start_gw_migration(Node* gw);
    void start_gw_snapshot(Node* gw);
    bool is_gateway(Node* node);
    void end_parent_precopy_if_needed(Node* gw);

};


#endif

