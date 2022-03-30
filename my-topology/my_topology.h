#ifndef my_topology_h
#define my_topology_h

#include "scheduler.h"
#include "simulator.h"
#include "timer-handler.h"
#include "topo_node.h"
#include "orch_stupid.h"
#include <vector>
#include <map>
#include <string>
#include <list>

#define NOW Scheduler::instance().clock()

const std::string BW = "5Mb"; 
const std::string DELAY = "3ms";
const std::string QUEUE_T = "DropTail";

class Tcl;
class TclObject;
class MigrationManager; 


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

    /**
     * @brief Get all the leaves in the subtree of this 
     * node. 
     *  
     * Any node that does not have any children is 
     * considered a leaf node. 
     *  
     * @param root The root of the subtree.
     * @return std::vector<Node*> of the leaves in the 
     * subtree.
     */
    std::vector<Node*> get_leaves(Node* root); 
    
    /**
     * @brief Get all the internal nodes in the subtree 
     * of this node. 
     *  
     * Any node that does have any children is 
     * considered an internal node. 
     *  
     * @param root The root of the subtree.
     * @return std::vector<Node*> of the internal nodes 
     * in the subtree.
     */
    std::vector<Node*> get_internal_nodes(Node* root); 
    

    /**
     * @brief Get the all siblings of this node in the 
     * subtree of the root node.
     * 
     * This includes the same node as well. 
     * 
     * @param node The node to get the siblings of.
     * @param root The root to compute the subtree.
     * @return std::vector<Node*> of the siblings of this
     * node.
     */
    std::vector<Node*> get_all_siblings(Node* node, Node* root);

    /**
     * @brief Get the next sibling of this node in the 
     * subtree of the root node. 
     * 
     * @param node the node to compute the sibling for.
     * @param root The root to compute the subtree.
     * @return Node* The next sibling. 
     * @return nullptr if there is no next sibling. 
     */
    Node* get_next_sibling(Node* node, Node* root);

    /**
     * @brief 
     * 
     * @param node 
     * @param n 
     * @return Node* 
     */
    Node* get_nth_parent(Node* node, int n);
    
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


    int get_subtree_depth(Node* root);

    

	virtual int command(int argc, const char*const* argv);

    void notify_flow_fin(Node* n);


private:

    void tcl_command(const std::list<std::string> & myArguments);
    
    void setup_apps();

    void make_peers(Node* n1, Node* n2);
    void connect_agents(Node* n1, Node* n2);
    void connect_nodes(Node* parent, Node* child);

    Node* make_node();
    Node* find_node(Node* root, std::vector<int> branch_ids); 
    void make_tree(Node* parent, std::vector<int> branching_ds); 
    void duplicate_tree(Node* src, Node* dst); 
    
    
    
    void add_link(Node* parent, Node* child); 

    // Migration 
    Node* mig_root; 
    Node* traffic_src; 
    
    // Core Data
    std::vector<Node*> nodes;
    std::map<Node*, TopoNode> data; 
    std::string sim_ptr; 
	  
    int verbose;  
    static MyTopology* instance_;
    MigrationManager* mig_manager_; 
    
    
    friend class Orchestrator; 
    friend class StupidOrchestrator; 

    // Useless functions 
    void send_data(Node* n1);
    void print_nodes();
    void print_graph();
    
};

#endif
