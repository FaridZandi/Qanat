#ifndef topo_node_h
#define topo_node_h

#include <vector>
#include <string>
#include "node.h"
#include "network_func.h"

enum OpMode { NoneMode, VM, GW };


class TopoNode{
public: 
    TopoNode();
    virtual ~TopoNode(); 

    /**
     * @brief Entry point for the packets to be processed by this node. 
     * Each node will have a list of NFs that the packet will go through. 
     * Each of the NFs might need asyncronous execution. Therefore, the 
     * execution of this function can be interrupted multipe times, 
     * depending on the number and behaviour of the NFs. 
     * 
     * The NFs that interrupt the execution of this function, will later 
     * call this function again with the apropriate starting position. 
     * (or they might never call it, if the packet should not continue
     * on this path anymore).
     * 
     * When the function reaches the end of the chain, it will call the 
     * rest of the recv function (recv2) in the classifier.  
     * 
     * @param p The packet. 
     * @param h The pcaket handler. 
     * @param start_pos The  position of the chain to start processing 
     * from. 
     */
    void process_packet(Packet* p, Handler* h, int start_pos = 0);

    /**
     * @brief Adds a new instance of some NF to the list of NFs. 
     * 
     * @param type Indicates the type of the NF to be created. 
     * @param parameter Indicates the parameter to be fed to the 
     * NF. For example, a buffer NF should have its size parameter
     * specified. 
     * 
     * @return NF* The pointer to the new instance. 
     */
    NF* add_nf(std::string type, double parameter = 0);

    /**
     * @brief Get the nth occurence of NFs of a specified type. 
     * 
     * @param type The type of the NF to be returned. 
     * @param occurence Specifies which instance of that type to 
     * be returned. Default case is returning the first occurence.
     * 
     * @return NF* The pointer to the instance. 
     */
    NF* get_nf(std::string type, int occurence = 1);

    /**
     * @brief removes all of the instances of the NFs of the 
     * specified type from the list of NFs. 
     * 
     * @param type The type of NFs to be removed.
     */
    void remove_nf(std::string type);

    /**
     * @brief prints some info about the NFs on this node.
     */
    void print_nfs();

    void print_stateful_nfs(); 

    StatefulNF* get_stateful_nf(); 

    /**
     * @brief add a node to the list of children.
     * 
     * @param n int: The node to be added. 
     */
    inline void add_child(int n) { children.push_back(n); }; 

    /**
     * @brief add a node to the list of parents. 
     * 
     * @param n int: The node to be added. 
     */
    inline void add_parent(int n) { parents.push_back(n); }

    /**
     * @brief Get the first parent of this node. 
     * 
     * For tree-like structures this is supposed to be only
     * parent this node has. 
     * 
     * @return int: The first parent.  
     */
    inline int first_parent(){ 
        if(parents.size() > 0){
            return parents[0];    
        } else {
            return -1; 
        }
    }


    std::vector<NF*> nfs; 

    std::string udp;
    std::string tcp;  
    std::string tcp_sink;  
    std::string app; 

    int uid; 
    int peer; 
    Node* node; 
    std::string pointer; 

    std::vector<int> children; 
    std::vector<int> parents;
    
    int layer_from_bottom; 
    
    bool bypass_processing; 
    OpMode mode; 

private:
    void rm_nf_at_index(int index);

};


#endif