#ifndef my_topology_h
#define my_topology_h

#include "scheduler.h"
#include "simulator.h"
#include "timer-handler.h"
#include "topo_node.h"
#include "orch_v1.h"
#include <vector>
#include <map>
#include <string>
#include <stack>
#include <list>

#define NOW Scheduler::instance().clock()

const std::string BW = "5Mb"; 
const std::string DELAY = "3ms";
const std::string QUEUE_T = "DropTail";

class Tcl;
class TclObject;
class MigrationManager; 


enum path_mode{
    PATH_MODE_SENDER,
    PATH_MODE_RECEIVER,
};

class MyTopology: public TclObject {

public:
    
    MyTopology(); 
    virtual ~MyTopology();


    /**
     * @brief will process this packet at this node. 
     * 
     * If there is a network function at this node, 
     * the packet will be handed to the NF for processing. 
     * 
     * The rest of the recv function (recv2) will be executed
     * after the NFs are done with the packet. 
     * 
     * @param p The packet to be processed.  
     * @param h The handler of the packet. 
     * @param node The node to process the packet. 
     */
    void process_packet(Packet* p, Handler*h, Node* node);

    
    
    void setup_nth_layer_tunnel(Node* vm, int n); 
    Node* get_nth_parent(Node* node, int n);
    TopoNode& get_data(Node* node); 
    int uid(Node* node); 
    Node* get_peer(Node* n); 
    std::vector<Node*> get_children(Node* n); 
    Node* get_node_by_address(int addr);
    std::vector<int> get_path(Node* n1, path_mode pm);
    Node* get_mig_root(); 
    std::vector<Node*>& get_used_nodes();

    std::vector<Node*> get_leaves(Node* root); 
    std::vector<Node*> get_internals(Node* root); 
    std::vector<Node*> get_all_nodes(Node* root); 

    /**
     * set the node pointer for all the classfiers 
     * of all the nodes in the topology. This is 
     * necessary for the packets to be preclassified
     * and processed at the nodes. 
     */
    void introduce_nodes_to_classifiers();


    /**
     * @brief globally accesible singeleton topology. 
     * 
     * @return the reference to this instance.
     */
    static MyTopology& instance() {
		return (*instance_);
	}

    /**
     * @brief access the migration manager of this topology
     * 
     * @return MigrationManager& the instance 
     */
    MigrationManager& mig_manager();

	virtual int command(int argc, const char*const* argv);

    void send_data(Node* n1, int n_bytes);

    void connect_agents(Node* n1, Node* n2);

    void print_graph(void (*print_node) (Node*));

    Node* storage_node; 
private:

    void tcl_command(const std::list<std::string> & myArguments);
    
    // higher level management (lvl 4 and 5 of network stack)
    void start_tcp_app(Node* n1);

    // Virtual tree managament
    Node* make_node(bool is_source);
    int make_tree(int parent, std::vector<int> branching_ds); 
    int find_node(int root, std::vector<int> branch_ids); 
    void make_peers(int n1, int n2);
    int duplicate_tree(int root); 
    void connect_nodes(int parent, int child);

    std::vector<Node*> get_subtree_nodes(Node* root, 
                                         bool include_leaves, 
                                         bool include_internals ); 


    void print_report(int interval);

    // Migration 
    Node* mig_root; 
    
    // Core Data
    std::stack<Node*> source_nodes;
    std::stack<std::string> source_nodes_ptrs;
    std::stack<Node*> dest_nodes;
    std::stack<std::string> dest_nodes_ptrs;

    std::vector<Node*> used_nodes;
    std::map<int, Node*> node; 
    std::map<Node*, TopoNode> data; 

    std::string sim_ptr; 
	  
    int verbose;  
    static MyTopology* instance_;
    static int toponode_uid_counter; 
    MigrationManager* mig_manager_; 
    
    

    // Useless functions 

    void print_tree_node(std::string prefix, 
                         Node* this_node, 
                         bool isLast, 
                         void (*print_node) (Node*));

    void print_nodes();


    //New stuff 

};

#endif
