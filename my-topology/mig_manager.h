#ifndef mig_manager_h
#define mig_manager_h

#include <map>

class Node; 
class Packet; 
class Handler;


struct tunnel_data { 
	bool valid; 

	Node* in;
	Node* out;

	Node* from; 
	Node* to; 

	int uid; 
};

enum Tunnel_Point {
    Tunnel_None, 
    Tunnel_In, 
    Tunnel_Out, 
    Tunnel_From, 
    Tunnel_To
};

enum Direction {
    Dir_None, 
    Incoming, 
    Outgoing, 
};



class MigrationManager {
public:
    MigrationManager(); 
    virtual ~MigrationManager(); 

    /**
     * @brief Applies the necessary changes to a packet 
     * if it matches any of the tunnels activated on this 
     * node. 
     * 
     * @param p The packet to pre classify.
     * @param h The Handler of the packet. 
     * @param n The node that currently processes the packet.
     * @return true if the packet should be processed by the
     * rest of the original classify function. 
     * @return false if the packet should be ignored by the 
     * calling classify function. 
     */
    virtual bool pre_classify(Packet* p, Handler* h, Node* n) = 0;
    
    /**
     * @brief Adds a tunnel to the current set of active
     * tunnels.
     * 
     * @param in The entry point of the tunnel.
     * @param out The exit point of the tunnel.
     * @param from Migration source.
     * @param to Migration destination.
     * 
     * @return The uid of this tunnel.
     */
    virtual int activate_tunnel(Node* in, Node* out, 
                        Node* from, Node* to) = 0;

    /**
     * @brief removes the tunnel from active tunnels.
     * 
     * @param uid The uid of the deactivating tunnel. 
     */
    virtual void deactivate_tunnel(int uid) = 0;

};

#endif
