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
    virtual bool pre_classify(Packet* p, Handler* h, Node* n);
    

    /**
     * @brief Checks if the further functions in the NF chain
     * should be applied to this packet or not. 
     * 
     * @param p The packet to determine bypass.
     * @param h The Handler of the packet. 
     * @param n The node that currently processes the packet.
     * @return true if the packet should bypass processsing. 
     * @return false if the packet should not bypass processing.  
     */
    bool bypass_processing(Packet* p, Handler* h, Node* n);
    

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
    int activate_tunnel(Node* in, Node* out, 
                        Node* from, Node* to);
    
    /**
     * @brief removes the tunnel from active tunnels.
     * 
     * @param uid The uid of the deactivating tunnel. 
     */
    virtual void deactivate_tunnel(int uid); 

protected: 

    virtual void add_tunnel(tunnel_data tunnel);    
    
    void log_packet(Packet* p); 
    void log_tunnel(tunnel_data td, Tunnel_Point tp, Packet* p);
    
    Tunnel_Point packet_match(tunnel_data, Packet*, Node*); 
    Direction packet_dir(tunnel_data, Packet*);
    
    bool tunnel_packet_in(tunnel_data, Packet*, Node*);
	bool tunnel_packet_out(tunnel_data, Packet*, Node*);
    bool handle_packet_from(tunnel_data, Packet*, Handler*, Node*);
    bool handle_packet_to(tunnel_data, Packet*, Handler*, Node*);

    tunnel_data* tunnels; 

    static int tunnel_uid_counter;
	static const int tunnel_count = 10; 

    bool verbose;
};


class EfficentMigrationManager: public MigrationManager{

public: 
    EfficentMigrationManager(); 

    virtual ~EfficentMigrationManager(); 

    virtual bool pre_classify(Packet* p, Handler* h, Node* n);

    virtual void deactivate_tunnel(int uid); 

protected: 

    virtual void add_tunnel(tunnel_data tunnel); 
private: 

    std::map<int, std::map<int, Tunnel_Point> > rules; 
    std::map<int, std::map<int, tunnel_data> > data; 
};

#endif
