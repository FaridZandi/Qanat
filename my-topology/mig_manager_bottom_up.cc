#include "mig_manager_bottom_up.h"
#include "my_topology.h"
#include "orchestrator.h"
#include "node.h"
#include "packet.h"
#include <iostream>
#include "tcp-full.h"
#include "utility.h"

int MigrationManagerBottomUp::tunnel_uid_counter = 0;
int MigrationManagerBottomUpSimplified::tunnel_uid_counter = 0;

MigrationManagerBottomUp::MigrationManagerBottomUp(){
	tunnels = new tunnel_data[tunnel_count]; 
	for (int i = 0; i < tunnel_count; i++){
		tunnels[i].valid = false;
	}
	verbose = MyTopology::verbose_mig;
	active_tunnels = 0;  
}

MigrationManagerBottomUp::~MigrationManagerBottomUp(){
	delete[] tunnels; 
}


int MigrationManagerBottomUp::activate_tunnel(Node* in, Node* out, 
                                 	  Node* from, Node* to) {   
	
	tunnel_data td; 
    
	td.valid = true;
	td.in = in; 
    td.out = out;   		
	td.from = from; 
    td.to = to; 

    td.uid = MigrationManagerBottomUp::tunnel_uid_counter ++;

    add_tunnel(td);                             

	// std::cout << "tunnel uid: " << td.uid << std::endl;
	return td.uid; 
}



void MigrationManagerBottomUp::deactivate_tunnel(int uid){
    for(int i = 0; i < MigrationManagerBottomUp::tunnel_count; i++){
		if(tunnels[i].uid == uid){
			tunnels[i].valid = false;
			break;
		}
	}
}


bool MigrationManagerBottomUp::should_ignore(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 

	if (iph->traffic_class == 2){
		// std::cout << "background traffic detected" << std::endl; 
		return true; 
	} else {
		// std::cout << "migration traffic detected" << std::endl; 
		return false; 
	}

}


void MigrationManagerBottomUp::add_tunnel(tunnel_data tunnel){
	for(int i = 0; i < MigrationManagerBottomUp::tunnel_count; i++){
		if (not tunnels[i].valid){
			tunnels[i] = tunnel;
			active_tunnels ++; 
            return; 
        }
	}
	std::cout << "Not enough space to store the tunnels!"; 
	std::cout << "Exiting!";
	exit(0); 
}

void MigrationManagerBottomUp::log_packet(Packet* p){
	if (not verbose){
		return; 
	}
	
	hdr_ip* iph = hdr_ip::access(p); 

	std::cout << "pre_classify "; 
    std::cout << iph->src_.addr_ << " to " << get_packet_dst(p) << " with prio " << iph->prio_ << " with class " << iph->traffic_class;
    std::cout << std::endl; 
}

void MigrationManagerBottomUp::log_tunnel(tunnel_data td, 
								  Tunnel_Point tp, 
								  Packet* p){
	if (not verbose){
		return; 
	}
	
	if (tp == Tunnel_Point::Tunnel_None){
		return; 
	}

	std::cout << "[tunnel " << td.uid << "] "; 
	
	if (tp == Tunnel_Point::Tunnel_From) {
		std::cout << "tunnel From detected"; 
	} else if (tp == Tunnel_Point::Tunnel_To) {
		std::cout << "tunnel To detected"; 
	} else if (tp == Tunnel_Point::Tunnel_In) {
		std::cout << "tunnel In detected"; 
	} else if (tp == Tunnel_Point::Tunnel_Out) {
		std::cout << "tunnel Out detected"; 
	}
	
    std::cout << std::endl; 
}


Tunnel_Point MigrationManagerBottomUp::packet_match(tunnel_data td, 
											Packet* p, Node* n){

	if (not td.valid){
		return Tunnel_Point::Tunnel_None; 
	}

	hdr_ip* iph = hdr_ip::access(p); 
	
	auto t_in = td.in->address(); 
	auto t_out = td.out->address(); 
	auto t_from = td.from->address(); 
	auto t_to = td.to->address(); 

	auto p_src = iph->src_.addr_;
	auto p_dst = get_packet_dst(p); 
	
	// std::cout << "packet=> "; 
	// std::cout << "src: " << p_src << " "; 
	// std::cout << "dst: " << p_dst; 
	// std::cout << std::endl; 

	auto n_addr = n->address(); 


	if (n_addr == t_in){ 										// this is the TUNNEL_IN node 
		if (p_dst == t_from or p_src == t_to){									// the packet is going to DST
			return Tunnel_Point::Tunnel_In; 				
		}
	} else if (n_addr == t_out){								// this is the TUNNEL_OUT node
		if (p_src == t_to or p_dst == t_to){									    // the packet is going to DST
			return Tunnel_Point::Tunnel_Out; 					// used for early redirectin of packets
		}	
	} else if (n_addr == t_from){								// this is the DST node
		if (p_dst == t_from or p_src == t_from){				// the packet was destined to DST
			return Tunnel_Point::Tunnel_From;					// or was generated there
		} 
	} else if (n_addr == t_to){									// this is the DST_NEW node
		if (p_dst == t_to){										// the packet was destined to DST_NEW
			return Tunnel_Point::Tunnel_To;
		}
	} 

	return Tunnel_Point::Tunnel_None; 
}


// This function is obsolete. Should not be used. 
Direction MigrationManagerBottomUp::packet_dir(tunnel_data td, 
									   Packet* p){
	// hdr_ip* iph = hdr_ip::access(p); 

	// auto t_in = td.in->address(); 
	// auto t_out = td.out->address(); 
	// auto t_src = td.src->address();
	// auto p_src = iph->src_.addr_;
	// auto p_dst = iph->dst_.addr_;

	// // TODO: make this independent of the source address
	// // Then we can support a tunnel with any source address. 
	// if (p_src == t_src){
	// 	return Direction::Incoming; 
	// } else if (p_src == t_in){
	// 	return Direction::Incoming; 
	// } else if (p_dst == t_src){
	// 	return Direction::Outgoing; 
	// } 

	return Direction::Dir_None; 
}

bool MigrationManagerBottomUp::pre_classify(Packet* p, Handler* h, Node* n){
	if (should_ignore(p)){
		return true; 
	}

	log_packet(p);

    for(int i = 0; i < MigrationManagerBottomUp::active_tunnels; i++){ 
		auto td = tunnels[i]; 
		Tunnel_Point tp = packet_match(td, p, n); 
		
		log_tunnel(td, tp, p);

		if (tp == Tunnel_Point::Tunnel_None) {
			continue; 
		} else if (tp == Tunnel_Point::Tunnel_In) {
			return tunnel_packet_in(td, p, n); 
		} else if (tp == Tunnel_Point::Tunnel_Out) {
			return tunnel_packet_out(td, p, n); 
		} else if (tp == Tunnel_Point::Tunnel_From) {
			return handle_packet_from(td, p, h, n);
		} else if (tp == Tunnel_Point::Tunnel_To) {
			return handle_packet_to(td, p, h, n); 
		} 
	}

	unset_high_prio(p);

    return true; 
}


int MigrationManagerBottomUp::get_packet_dst(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 

	auto p_dst = iph->dst_.addr_;

	if (iph->gw_path_pointer != -1){
		p_dst = iph->gw_path[0];	
	}

	return p_dst; 	
}

void MigrationManagerBottomUp::set_packet_src(Packet* p, int src){
	hdr_ip* iph = hdr_ip::access(p); 
	iph->src_.addr_ = src; 
}



bool MigrationManagerBottomUp::tunnel_packet_in(tunnel_data td, 
								  		Packet*p, Node* n){			  

	// The packet should be tunneled to the tunnel_out
	// point of this tunnel. The original source and 
	// destination of this packet are recorded in temp
	// variables in their ip header. 

	convert_path(p);
	add_to_path(p, td.out->address());
	set_high_prio(p); 
	
	MyTopology::instance().inc_tunnelled_packets();

    return true; 	 
}

bool MigrationManagerBottomUp::tunnel_packet_out(tunnel_data td, 
								   Packet*p, Node* n){

	// Packet has reached the end of the tunnel. 
	// Original source will be recovered, but instead
	// of the original destination, the new destination
	// will be assigned to the packet. 	

	unset_high_prio(p); 

    return true; 
}

bool MigrationManagerBottomUp::handle_packet_from(tunnel_data td, 
                                      Packet*p, Handler* h, 
                                      Node* n){
    
	// If the packet is sent from the source to the
	// destination, it either: 
	// 1. Has it's agent flag on, which means it had
	// arrived at the new destination and was redirected
	// back to the orignial destination for processing.
	// So we don't do anything with it, and will let it 
	// reach the agents. 
	// 2. Doesn't have an agent flag, so it means it has
	// arrived directly from the source to the destination. 
	// It might seem that this packet should have been 
	// tunneled at the tunnel_in point of this tunnel, but
	// when this packet passed through that point, the 
	// tunnel was not established yet. Therefore, it 
	// should be tunneled to the new destination with a 
	// high priority. 
	// If the packet is sent from the source to the 
	// destination, it should be handed back to the new 
	// destination to deliver it.   

	hdr_ip* iph = hdr_ip::access(p);
	auto p_src = iph->src_.addr_;
	auto p_dst = get_packet_dst(p);
	auto n_addr = n->address(); 


	Direction dir = Direction::Dir_None; 
	if (p_dst == n_addr){
		dir = Direction::Incoming;
	} else if (p_src == n_addr){
		dir = Direction::Outgoing;
	}

	auto& orch = BaseOrchestrator::instance();
	auto node_state = orch.get_mig_state(n);

	if (dir == Direction::Outgoing) { 
		if (node_state == MigState::Migrated or 
			node_state == MigState::InMig) {

			iph->src_.addr_ = td.to->address(); 
			td.to->get_classifier()->recv(p, h);
			return false;  
		} else {	
			return true; 
		}
	} else if (dir == Direction::Incoming) {		 
		if (node_state == MigState::Migrated or 
			node_state == MigState::InMig) {	
			
			// std::cout << "MigrationManagerBottomUp::handle_packet_from: " << n->address() << std::endl;
			iph->dst_.addr_ = td.to->address(); 
			set_high_prio(p); 
			MyTopology::instance().inc_tunnelled_packets();
			return true; 	
		} else {

			if (iph->skip_first_mngr_flag == false){
				iph->skip_first_mngr_flag = true;
				return true; 
			}
			
			n->get_classifier()->recv2(p, h); 
			return false; 
		}
	}

	std::cout << "packets should not reach here." << std::endl;
	return true;
}

bool MigrationManagerBottomUp::handle_packet_to(tunnel_data td, 
										Packet*p, 
										Handler* h,
										Node* n){
	hdr_ip* iph = hdr_ip::access(p); 	

	if (iph->skip_first_mngr_flag == false){
		iph->skip_first_mngr_flag = true;
		return true; 
	}
	iph->skip_first_mngr_flag = false;

	auto& orch = BaseOrchestrator::instance();
	auto node_state = orch.get_mig_state(n);

	if (node_state == MigState::Normal){
		iph->dst_.addr_ = td.from->address(); 		
		td.from->get_classifier()->recv2(p, h); 
		return false; 
	} else {
		return true; 
	}
}


MigrationManagerBottomUpSimplified::MigrationManagerBottomUpSimplified()
    : verbose_(MyTopology::verbose_mig) {}

MigrationManagerBottomUpSimplified::~MigrationManagerBottomUpSimplified() = default;

bool MigrationManagerBottomUpSimplified::pre_classify(Packet* p, Handler* h, Node* n){
	if (should_ignore(p)){
		return true; 
	}

	log_packet(p);

	auto endpoint_it = endpoint_bindings_.find(n);
	if (endpoint_it != endpoint_bindings_.end()) {
		for (const auto& binding : endpoint_it->second) {
			if (binding.tunnel_index >= tunnels_.size()){
				continue;
			}
			auto& td = tunnels_[binding.tunnel_index];
			if (not td.valid){
				continue;
			}

			if (binding.role == Tunnel_Point::Tunnel_From) {
				if (not matches_from(td, p, n)){
					continue;
				}
				log_tunnel(td, Tunnel_Point::Tunnel_From, p);
				return handle_from(td, p, h, n);
			} else if (binding.role == Tunnel_Point::Tunnel_To) {
				if (not matches_to(td, p, n)){
					continue;
				}
				log_tunnel(td, Tunnel_Point::Tunnel_To, p);
				return handle_to(td, p, h, n);
			}
		}
	}

	auto in_it = transit_in_bindings_.find(n);
	if (in_it != transit_in_bindings_.end()) {
		for (auto idx : in_it->second) {
			if (idx >= tunnels_.size()){
				continue;
			}
			auto& td = tunnels_[idx];
			if (not td.valid){
				continue;
			}
			if (not matches_in(td, p, n)){
				continue;
			}
			log_tunnel(td, Tunnel_Point::Tunnel_In, p);
			return handle_in(td, p, n);
		}
	}

	auto out_it = transit_out_bindings_.find(n);
	if (out_it != transit_out_bindings_.end()) {
		for (auto idx : out_it->second) {
			if (idx >= tunnels_.size()){
				continue;
			}
			auto& td = tunnels_[idx];
			if (not td.valid){
				continue;
			}
			if (not matches_out(td, p, n)){
				continue;
			}
			log_tunnel(td, Tunnel_Point::Tunnel_Out, p);
			return handle_out(td, p, n);
		}
	}

	unset_high_prio(p);
	return true;
}

int MigrationManagerBottomUpSimplified::activate_tunnel(Node* in, Node* out, Node* from, Node* to){
	tunnel_data td{};
	td.valid = true;
	td.in = in;
	td.out = out;
	td.from = from;
	td.to = to;
	td.uid = MigrationManagerBottomUpSimplified::tunnel_uid_counter++;

	size_t index = tunnels_.size();
	tunnels_.push_back(td);
	uid_to_index_[td.uid] = index;

	transit_in_bindings_[in].push_back(index);
	transit_out_bindings_[out].push_back(index);
	add_endpoint_binding(from, Tunnel_Point::Tunnel_From, index);
	add_endpoint_binding(to, Tunnel_Point::Tunnel_To, index);

	return td.uid;
}

void MigrationManagerBottomUpSimplified::deactivate_tunnel(int uid){
	auto uid_it = uid_to_index_.find(uid);
	if (uid_it == uid_to_index_.end()){
		return;
	}

	size_t index = uid_it->second;
	if (index >= tunnels_.size()){
		uid_to_index_.erase(uid_it);
		return;
	}

	auto& td = tunnels_[index];
	if (not td.valid){
		uid_to_index_.erase(uid_it);
		return;
	}

	remove_endpoint_binding(td.from, Tunnel_Point::Tunnel_From, index);
	remove_endpoint_binding(td.to, Tunnel_Point::Tunnel_To, index);

	remove_transit_binding(transit_in_bindings_, td.in, index);
	remove_transit_binding(transit_out_bindings_, td.out, index);

	td.valid = false;
	uid_to_index_.erase(uid_it);
}

bool MigrationManagerBottomUpSimplified::should_ignore(Packet* p){
	hdr_ip* iph = hdr_ip::access(p);

	if (iph->traffic_class == 2){
		return true;
	}

	return false;
}

void MigrationManagerBottomUpSimplified::log_packet(Packet* p){
	if (not verbose_){
		return;
	}

	hdr_ip* iph = hdr_ip::access(p);

	std::cout << "pre_classify ";
	std::cout << iph->src_.addr_ << " to " << get_packet_dst(p) << " with prio " << iph->prio_ << " with class " << iph->traffic_class;
	std::cout << std::endl;
}

void MigrationManagerBottomUpSimplified::log_tunnel(const tunnel_data& td, Tunnel_Point tp, Packet* p){
	if (not verbose_){
		return;
	}

	if (tp == Tunnel_Point::Tunnel_None){
		return;
	}

	std::cout << "[tunnel " << td.uid << "] ";

	if (tp == Tunnel_Point::Tunnel_From) {
		std::cout << "tunnel From detected";
	} else if (tp == Tunnel_Point::Tunnel_To) {
		std::cout << "tunnel To detected";
	} else if (tp == Tunnel_Point::Tunnel_In) {
		std::cout << "tunnel In detected";
	} else if (tp == Tunnel_Point::Tunnel_Out) {
		std::cout << "tunnel Out detected";
	}

	std::cout << std::endl;
}

int MigrationManagerBottomUpSimplified::get_packet_dst(Packet* p){
	hdr_ip* iph = hdr_ip::access(p);

	auto p_dst = iph->dst_.addr_;

	if (iph->gw_path_pointer != -1){
		p_dst = iph->gw_path[0];
	}

	return p_dst;
}

bool MigrationManagerBottomUpSimplified::matches_in(const tunnel_data& td, Packet* p, Node* n){
	if (not td.valid){
		return false;
	}
	if (td.in != n){
		return false;
	}
	hdr_ip* iph = hdr_ip::access(p);
	auto p_dst = get_packet_dst(p);
	auto p_src = iph->src_.addr_;
	return (p_dst == td.from->address() || p_src == td.to->address());
}

bool MigrationManagerBottomUpSimplified::matches_out(const tunnel_data& td, Packet* p, Node* n){
	if (not td.valid){
		return false;
	}
	if (td.out != n){
		return false;
	}
	hdr_ip* iph = hdr_ip::access(p);
	auto p_dst = get_packet_dst(p);
	auto p_src = iph->src_.addr_;
	return (p_src == td.to->address() || p_dst == td.to->address());
}

bool MigrationManagerBottomUpSimplified::matches_from(const tunnel_data& td, Packet* p, Node* n){
	if (not td.valid){
		return false;
	}
	if (td.from != n){
		return false;
	}
	hdr_ip* iph = hdr_ip::access(p);
	auto n_addr = n->address();
	auto p_dst = get_packet_dst(p);
	auto p_src = iph->src_.addr_;
	return (p_dst == n_addr || p_src == n_addr);
}

bool MigrationManagerBottomUpSimplified::matches_to(const tunnel_data& td, Packet* p, Node* n){
	if (not td.valid){
		return false;
	}
	if (td.to != n){
		return false;
	}
	auto p_dst = get_packet_dst(p);
	return (p_dst == td.to->address());
}

bool MigrationManagerBottomUpSimplified::handle_in(const tunnel_data& td, Packet* p, Node* n){
	convert_path(p);
	add_to_path(p, td.out->address());
	set_high_prio(p);
	MyTopology::instance().inc_tunnelled_packets();
	return true;
}

bool MigrationManagerBottomUpSimplified::handle_out(const tunnel_data& td, Packet* p, Node* n){
	unset_high_prio(p);
	return true;
}

bool MigrationManagerBottomUpSimplified::handle_from(const tunnel_data& td, Packet* p, Handler* h, Node* n){
	hdr_ip* iph = hdr_ip::access(p);
	auto p_src = iph->src_.addr_;
	auto p_dst = get_packet_dst(p);
	auto n_addr = n->address();

	Direction dir = Direction::Dir_None;
	if (p_dst == n_addr){
		dir = Direction::Incoming;
	} else if (p_src == n_addr){
		dir = Direction::Outgoing;
	}

	auto& orch = BaseOrchestrator::instance();
	auto node_state = orch.get_mig_state(n);

	if (dir == Direction::Outgoing) {
		if (node_state == MigState::Migrated || node_state == MigState::InMig) {
			iph->src_.addr_ = td.to->address();
			td.to->get_classifier()->recv(p, h);
			return false;
		}
		return true;
	} else if (dir == Direction::Incoming) {
		if (node_state == MigState::Migrated || node_state == MigState::InMig) {
			iph->dst_.addr_ = td.to->address();
			set_high_prio(p);
			MyTopology::instance().inc_tunnelled_packets();
			return true;
		}

		if (iph->skip_first_mngr_flag == false){
			iph->skip_first_mngr_flag = true;
			return true;
		}

		n->get_classifier()->recv2(p, h);
		return false;
	}

	return true;
}

bool MigrationManagerBottomUpSimplified::handle_to(const tunnel_data& td, Packet* p, Handler* h, Node* n){
	hdr_ip* iph = hdr_ip::access(p);

	if (iph->skip_first_mngr_flag == false){
		iph->skip_first_mngr_flag = true;
		return true;
	}
	iph->skip_first_mngr_flag = false;

	auto& orch = BaseOrchestrator::instance();
	auto node_state = orch.get_mig_state(n);

	if (node_state == MigState::Normal){
		iph->dst_.addr_ = td.from->address();
		td.from->get_classifier()->recv2(p, h);
		return false;
	}

	return true;
}

void MigrationManagerBottomUpSimplified::add_endpoint_binding(Node* node, Tunnel_Point role, size_t tunnel_index){
	if (node == nullptr){
		return;
	}
	endpoint_bindings_[node].push_back({role, tunnel_index});
}

void MigrationManagerBottomUpSimplified::remove_endpoint_binding(Node* node, Tunnel_Point role, size_t tunnel_index){
	if (node == nullptr){
		return;
	}
	auto it = endpoint_bindings_.find(node);
	if (it == endpoint_bindings_.end()){
		return;
	}
	auto& vec = it->second;
	vec.erase(std::remove_if(vec.begin(), vec.end(), [role, tunnel_index](const EndpointBinding& binding){
		return binding.role == role && binding.tunnel_index == tunnel_index;
	}), vec.end());
	if (vec.empty()){
		endpoint_bindings_.erase(it);
	}
}

void MigrationManagerBottomUpSimplified::remove_transit_binding(std::unordered_map<Node*, std::vector<size_t>>& map, Node* node, size_t tunnel_index){
	if (node == nullptr){
		return;
	}
	auto it = map.find(node);
	if (it == map.end()){
		return;
	}
	auto& vec = it->second;
	vec.erase(std::remove(vec.begin(), vec.end(), tunnel_index), vec.end());
	if (vec.empty()){
		map.erase(it);
	}
}
