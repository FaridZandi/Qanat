#ifndef mig_manager_bottom_up_h
#define mig_manager_bottom_up_h

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>
#include "mig_manager.h"

class Node; 
class Packet; 
class Handler;


class MigrationManagerBottomUp : public MigrationManager {
public:
    MigrationManagerBottomUp(); 
    virtual ~MigrationManagerBottomUp(); 

    virtual bool pre_classify(Packet* p, Handler* h, Node* n);
    
    virtual int activate_tunnel(Node* in, Node* out, 
                                Node* from, Node* to);

    
    virtual void deactivate_tunnel(int uid); 

protected: 
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

class MigrationManagerBottomUpSimplified : public MigrationManager {
public:
    MigrationManagerBottomUpSimplified();
    ~MigrationManagerBottomUpSimplified() override;

    bool pre_classify(Packet* p, Handler* h, Node* n) override;
    int activate_tunnel(Node* in, Node* out, Node* from, Node* to) override;
    void deactivate_tunnel(int uid) override;

private:
    struct EndpointBinding {
        Tunnel_Point role;
        size_t tunnel_index;
    };

    bool should_ignore(Packet* p);
    void log_packet(Packet* p);
    void log_tunnel(const tunnel_data& td, Tunnel_Point tp, Packet* p);

    int get_packet_dst(Packet* p);

    bool matches_in(const tunnel_data& td, Packet* p, Node* n);
    bool matches_out(const tunnel_data& td, Packet* p, Node* n);
    bool matches_from(const tunnel_data& td, Packet* p, Node* n);
    bool matches_to(const tunnel_data& td, Packet* p, Node* n);

    bool handle_in(const tunnel_data& td, Packet* p, Node* n);
    bool handle_out(const tunnel_data& td, Packet* p, Node* n);
    bool handle_from(const tunnel_data& td, Packet* p, Handler* h, Node* n);
    bool handle_to(const tunnel_data& td, Packet* p, Handler* h, Node* n);

    void add_endpoint_binding(Node* node, Tunnel_Point role, size_t tunnel_index);
    void remove_endpoint_binding(Node* node, Tunnel_Point role, size_t tunnel_index);
    void remove_transit_binding(std::unordered_map<Node*, std::vector<size_t>>& map, Node* node, size_t tunnel_index);

    static int tunnel_uid_counter;

    std::vector<tunnel_data> tunnels_;
    std::unordered_map<int, size_t> uid_to_index_;
    std::unordered_map<Node*, std::vector<EndpointBinding>> endpoint_bindings_;
    std::unordered_map<Node*, std::vector<size_t>> transit_in_bindings_;
    std::unordered_map<Node*, std::vector<size_t>> transit_out_bindings_;

    bool verbose_;
};

#endif
