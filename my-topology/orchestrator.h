#ifndef orchestrator_h
#define orchestrator_h

class Node; 


enum MigState { NoMigState, PreMig, InMig, Migrated};

/*
The abstract BaseOrchestrator class. Provides the interface
for an orchestrator. Mainly inherited by the Orchestrator
class. Other orchestators can be added later, to use as 
baselines in our experiments, or to compare approaches. 
*/ 
class BaseOrchestrator{
public: 

    static BaseOrchestrator& instance();
    
    BaseOrchestrator() {}

    // signals 
    virtual void start_migration(){};
    virtual void vm_precopy_finished(Node* node){};
    virtual void vm_migration_finished(Node* node){};
    virtual void gw_snapshot_sent(Node* node){}; 
    virtual void gw_diff_sent(Node* node){}; 
    virtual void gw_sent_last_packet(Node* node1, Node* node2){}; 
    virtual void gw_received_last_packet(Node* node){}; 
    virtual void setup_nodes(){}; 
    virtual void start_background_traffic(){};
    
protected: 

    void initiate_data_transfer(Node* node, int size, 
                                void (*callback) (Node*));

};


// Main Orchestrator class. To implement the real protocol.
class Orchestrator : public BaseOrchestrator {
public: 
    // Singleton access to the orchestrator
	static Orchestrator& instance() {
        static Orchestrator instance; 
        return instance; 
	}
    
    Orchestrator(); 
    virtual ~Orchestrator(); 

    virtual void start_migration(){};
    virtual void vm_precopy_finished(Node* node){};
    virtual void vm_migration_finished(Node* node){};
    virtual void gw_snapshot_sent(Node* node){}; 
    virtual void gw_diff_sent(Node* node){}; 
    virtual void gw_sent_last_packet(Node* node1, Node* node2){}; 
    virtual void gw_received_last_packet(Node* node){}; 
    virtual void setup_nodes(){}; 
    virtual void start_background_traffic(){};
};





#endif
