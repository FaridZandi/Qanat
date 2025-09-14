#ifndef mig_manager_top_down_h
#define mig_manager_top_down_h

#include <map>
#include "mig_manager.h"

class Node; 
class Packet; 
class Handler;


class MigrationManagerTopDown : public MigrationManager {
public:
    MigrationManagerTopDown(); 
    virtual ~MigrationManagerTopDown(); 

    virtual bool pre_classify(Packet* p, Handler* h, Node* n);
    
    virtual int activate_tunnel(Node* in, Node* out, 
                        Node* from, Node* to);

    

    virtual void deactivate_tunnel(int uid); 

protected: 
    void handle_non_ready_nodes(Packet* p, Node* n);

    bool should_ignore(Packet* p); 

    void add_tunnel(tunnel_data tunnel);    
    
    void log_packet(Packet* p); 
    
    void log_tunnel(tunnel_data td, Tunnel_Point tp, Packet* p);

    int get_packet_dst(Packet* p);
    void set_packet_src(Packet* p, int src);

    Tunnel_Point packet_match(tunnel_data, Packet*, Node*); 
    Direction packet_dir(tunnel_data, Packet*);
    
    bool tunnel_packet_in(tunnel_data, Packet*, Node*);
	bool tunnel_packet_out(tunnel_data, Packet*, Node*);
    bool handle_packet_from(tunnel_data, Packet*, Handler*, Node*);
    bool handle_packet_to(tunnel_data, Packet*, Handler*, Node*);

    tunnel_data* tunnels; 

    static int tunnel_uid_counter;
	static const int tunnel_count = 100; 

    // temp. remove this later. 
    int active_tunnels; 

    bool verbose;
};

#endif
