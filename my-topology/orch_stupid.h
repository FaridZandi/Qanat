#ifndef orch_stupid_h
#define orch_stupid_h

#include "orchestrator.h"
#include "timer-handler.h"
#include <queue>
#include <map>


class Node; 

class VMSnapshotSender: public TimerHandler {
public:
    VMSnapshotSender() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* vm;
};

class VMPrecopySender: public TimerHandler {
public:
    VMPrecopySender() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* vm;
};

class GWDiffSender: public TimerHandler {
public:
    GWDiffSender() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* gw;
};

class GWSnapshotSender: public TimerHandler {
public:
    GWSnapshotSender() : TimerHandler() { }
    inline virtual void expire(Event*); 
    Node* gw;
};

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



enum MigState { NoMigState, PreMig, InMig, Migrated};


// Test version of Orchestrator
class StupidOrchestrator : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static StupidOrchestrator& instance() {
        static StupidOrchestrator instance; 
        return instance; 
	}
    
    StupidOrchestrator(); 
    virtual ~StupidOrchestrator(); 

    virtual void start_background_traffic(){};
    virtual void setup_node_types(); 
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

    // utility
    
    void setup_nth_layer_tunnel(Node* vm, int n); 
    
    void print_time();
    double random_wait(); 

    // data 

    std::queue<Node*> vm_migration_queue;
    std::map<Node*, MigState> mig_state; 

    static const int parallel_migrations = 2; 
};


#endif

