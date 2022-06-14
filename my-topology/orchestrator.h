#ifndef orchestrator_h
#define orchestrator_h

#include <string>
#include <list>
#include <map>
#include <queue>

class Node; 

struct nf_spec{
    std::string type; 
    double parameter; 
};


enum class MigState { 
    Normal, 
    PreMig, 
    InMig, 
    Buffering, 
    Migrated, 
    OutOfService,
    Uninitialized
};

/*
The abstract BaseOrchestrator class. Provides the interface
for an orchestrator. Mainly inherited by the Orchestrator
class. Other orchestators can be added later, to use as 
baselines in our experiments, or to compare approaches. 
*/ 
class BaseOrchestrator{
public: 

    static BaseOrchestrator& instance();
    
    BaseOrchestrator();

    // signals 
    virtual void start_migration(){};
    virtual void setup_nodes(); 

    MigState get_mig_state(Node* node);  
    std::string get_mig_state_string(Node* node);  
protected: 

    void initiate_data_transfer(Node* node, int size, 
                                void (*callback) (Node*));

    void log_event(std::string message, Node* node = nullptr, int arg = -1, bool print_tree = true);

    void buffer_on_peer(Node* node); 

    void process_on_peer(Node* node); 

    virtual std::list<nf_spec> get_vm_nf_list(){};

    virtual std::list<nf_spec> get_gw_nf_list(){};

    double random_wait();

    void set_node_state(Node* node, MigState state);

    void set_peer_state(Node* node, MigState state);

    std::queue<Node*> vm_migration_queue;
    std::queue<Node*> migration_queue;
    std::queue<Node*> gw_migration_queue;
    
    int in_migration_gws;
    int in_migration_vms; 
    int in_migration_nodes; 
    
    std::map<Node*, MigState> mig_state; 
};


#endif
