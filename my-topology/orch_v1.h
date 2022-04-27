#ifndef orch_v1_h
#define orch_v1_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>


class Node; 

class LastPacketSender : public TimerHandler {
public:
    LastPacketSender() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* gw;
};

class LastPacketReceiver : public TimerHandler {
public:
    LastPacketReceiver() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* gw;
};



// Test version of Orchestrator
class OrchestratorV1 : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static OrchestratorV1& instance() {
        static OrchestratorV1 instance; 
        return instance; 
	}
    
    OrchestratorV1(); 
    virtual ~OrchestratorV1(); 

    virtual void start_background_traffic(){};
    virtual void start_migration();

    virtual void vm_precopy_finished(Node* vm); 
    virtual void vm_migration_finished(Node* vm); 
    virtual void gw_snapshot_sent(Node* gw); 
    virtual void gw_diff_sent(Node* gw); 
    virtual void gw_sent_last_packet(Node* gw); 
    virtual void gw_received_last_packet(Node* gw); 

private: 

    void start_vm_precopy(Node* vm); 
    void start_vm_migration(Node* vm); 
    void start_gw_snapshot(Node* gw); 
    void start_gw_diff(Node* gw);  
    void copy_gw_state(Node* gw){}; 
    void mark_last_packet(Node* parent, Node* child); 
    void start_gw_migration_if_possible(Node* gw);     

    virtual std::list<nf_spec> get_vm_nf_list();
    virtual std::list<nf_spec> get_gw_nf_list();
};


#endif

