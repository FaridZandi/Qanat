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


enum MigState { 
    Normal, 
    PreMig, 
    InMig, 
    Buffering, 
    Migrated, 
    OutOfService
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
    virtual void vm_precopy_finished(Node* node){};
    virtual void vm_migration_finished(Node* node){};
    virtual void gw_snapshot_send_ack_from_peer(Node* node){};
    virtual void gw_start_processing_buffer_on_peer(Node* node){};
    virtual void gw_snapshot_ack_rcvd(Node* node){};
    virtual void gw_diff_sent(Node* node){}; 
    virtual void gw_sent_last_packet(Node* node1, Node* node2){}; 
    virtual void gw_received_last_packet(Node* node){}; 
    virtual void setup_nodes(); 
    virtual void start_background_traffic(){};
    virtual void migration_finished(); 

    MigState get_mig_state(Node* node);  
    std::string get_mig_state_string(Node* node);  
protected: 

    void initiate_data_transfer(Node* node, int size, 
                                void (*callback) (Node*));

    void tunnel_subtree_tru_parent(Node* node);

    void log_event(std::string message, Node* node = nullptr, int arg = -1, bool print_tree = true);

    void buffer_on_peer(Node* node); 

    void process_on_peer(Node* node); 

    virtual std::list<nf_spec> get_vm_nf_list(){};

    virtual std::list<nf_spec> get_gw_nf_list(){};

    double random_wait();

    void set_node_state(Node* node, MigState state);

    void set_peer_state(Node* node, MigState state);

    std::queue<Node*> vm_migration_queue;

    std::map<Node*, MigState> mig_state; 
};


#endif
