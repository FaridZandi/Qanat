#include "mig_manager.h"
#include "my_topology.h"
#include "node.h"
#include "packet.h"
#include <iostream>
#include "tcp-full.h"

int MigrationManager::tunnel_uid_counter = 0;

MigrationManager::MigrationManager(){
	tunnels = new tunnel_data[tunnel_count]; 
	for (int i = 0; i < tunnel_count; i++){
		tunnels[i].valid = false;
	}
	verbose = MyTopology::verbose_mig;
	active_tunnels = 0;  
}

MigrationManager::~MigrationManager(){
	delete[] tunnels; 
}


int MigrationManager::activate_tunnel(Node* in, Node* out, 
                                 	  Node* from, Node* to) {   
	
	tunnel_data td; 
    
	td.valid = true;
	td.in = in; 
    td.out = out;   		
	td.from = from; 
    td.to = to; 

    td.uid = MigrationManager::tunnel_uid_counter ++;

    add_tunnel(td);                             

	// std::cout << "tunnel uid: " << td.uid << std::endl;
	return td.uid; 


}



void MigrationManager::deactivate_tunnel(int uid){
    for(int i = 0; i < MigrationManager::tunnel_count; i++){
		if(tunnels[i].uid == uid){
			tunnels[i].valid = false;
			break;
		}
	}
}


bool MigrationManager::should_ignore(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 
	hdr_tcp* tcph = hdr_tcp::access(p); 

	if (iph->traffic_class == 2){
		// std::cout << "background traffic detected" << std::endl; 
		return true; 
	} else {
		// std::cout << "migration traffic detected" << std::endl; 
		return false; 
	}

}


void MigrationManager::add_tunnel(tunnel_data tunnel){
	for(int i = 0; i < MigrationManager::tunnel_count; i++){
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

void MigrationManager::log_packet(Packet* p){
	if (not verbose){
		return; 
	}
	
	hdr_ip* iph = hdr_ip::access(p); 

	std::cout << "pre_classify "; 
    std::cout << iph->src_.addr_ << " to " << iph->dst_.addr_ << " with prio " << iph->prio_ << " with class " << iph->traffic_class;
    std::cout << std::endl; 
}

void MigrationManager::log_tunnel(tunnel_data td, 
								  Tunnel_Point tp, 
								  Packet* p){
	if (not verbose){
		return; 
	}
	
	if (tp == Tunnel_Point::Tunnel_None){
		return; 
	}

	std::cout << "[tunnel " << td.uid << "] "; 
	
	if (tp == Tunnel_Point::Tunnel_In) {
		std::cout << "tunnel In detected"; 
	} else if (tp == Tunnel_Point::Tunnel_Out) {
		std::cout << "tunnel Out detected"; 
	} else if (tp == Tunnel_Point::Tunnel_From) {
		std::cout << "tunnel From detected"; 
	} else if (tp == Tunnel_Point::Tunnel_To) {
		std::cout << "tunnel To detected"; 
	}
	
    std::cout << std::endl; 
}


Tunnel_Point MigrationManager::packet_match(tunnel_data td, 
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
Direction MigrationManager::packet_dir(tunnel_data td, 
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

bool MigrationManager::bypass_processing(Packet* p, Handler* h, Node* n){
	for(int i = 0; i < MigrationManager::tunnel_count; i++){ 
		auto td = tunnels[i]; 
		Tunnel_Point tp = packet_match(td, p, n); 
		
		if (tp == Tunnel_Point::Tunnel_None) {
			return false; 
		} else {
			return true;
		}

	}
}


bool MigrationManager::pre_classify(Packet* p, Handler* h, Node* n){
	if (should_ignore(p)){
		return true; 
	}

	log_packet(p);

    for(int i = 0; i < MigrationManager::active_tunnels; i++){ 
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
    return true; 
}


void MigrationManager::convert_path(Packet* p){
	auto& topo = MyTopology::instance();
	hdr_ip* iph = hdr_ip::access(p); 

	for (int i = iph->gw_path_pointer; i >= 0; i --){
		int addr = iph->gw_path[i]; 
		auto node = topo.get_node_by_address(addr); 

		if (node != nullptr){
			iph->gw_path[i] = topo.get_peer(node)->address(); 
		} 	
	}
}

void MigrationManager::add_to_path(Packet* p, int addr){

	auto& topo = MyTopology::instance();
	hdr_ip* iph = hdr_ip::access(p); 

	iph->gw_path_pointer += 1; 
	iph->gw_path[iph->gw_path_pointer] = addr;
}


void MigrationManager::set_prio(Packet* p, int prio){
	auto& topo = MyTopology::instance();
	hdr_ip* iph = hdr_ip::access(p); 

	if (topo.enable_prioritization){
		if (prio == 1){
			iph->is_high_prio = true;
		} else {
			iph->is_high_prio = false; 
		}
	} else {
		// do nothing
	}
}


int MigrationManager::get_prio(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 
	return iph->prio_;
}



int MigrationManager::get_packet_dst(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 

	auto p_dst = iph->dst_.addr_;

	if (iph->gw_path_pointer != -1){
		p_dst = iph->gw_path[0];	
	}

	return p_dst; 	
}

void MigrationManager::set_packet_src(Packet* p, int src){
	hdr_ip* iph = hdr_ip::access(p); 
	iph->src_.addr_ = src; 
}



bool MigrationManager::tunnel_packet_in(tunnel_data td, 
								  Packet*p, Node* n){			  

	// The packet should be tunneled to the tunnel_out
	// point of this tunnel. The original source and 
	// destination of this packet are recorded in temp
	// variables in their ip header. 

	convert_path(p);
	add_to_path(p, td.out->address());
	set_prio(p, 1); 

    return true; 	 
}

bool MigrationManager::tunnel_packet_out(tunnel_data td, 
								   Packet*p, Node* n){

	// Packet has reached the end of the tunnel. 
	// Original source will be recovered, but instead
	// of the original destination, the new destination
	// will be assigned to the packet. 
	
	set_prio(p, 0); 

    return true; 
}

bool MigrationManager::handle_packet_from(tunnel_data td, 
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
	Direction dir; 

	auto t_from = td.from->address(); 
	auto p_src = iph->src_.addr_;

	auto p_dst = get_packet_dst(p);
	auto n_addr = n->address(); 

	if (p_dst == n_addr){
		dir = Direction::Incoming;
	} else if (p_src == n_addr){
		dir = Direction::Outgoing;
	}
	// std::cout << "Here" << std::endl; 

    if(iph->agent_tunnel_flag){
        return true; // don't do anything 
    } else {
        if (dir == Direction::Outgoing){ 
            iph->src_.addr_ = td.to->address(); 
			convert_path(p); 
            td.to->get_classifier()->recv(p, h);
            return false;  

        } else if (dir == Direction::Incoming){ 
            iph->dst_.addr_ = td.to->address(); 
            iph->prio_ = 15;
            return true;         
        }
    }
}

bool MigrationManager::handle_packet_to(tunnel_data td, Packet*p, 
										Handler* h, Node* n){

	// Turn on the tunnel flag, to differentiate it 
	// from the traffic that had directly been sent
	// to the dstination from the srouce, but wasn't
	// tunneled in the gateway level, since the packet
	// had already passed the gateway when the tunnel 
	// was established. 

    hdr_ip* iph = hdr_ip::access(p); 	

    iph->dst_.addr_ = td.from->address(); 
    iph->agent_tunnel_flag = true;
    
	td.from->get_classifier()->recv(p, h); 

    return false; 
}

/**********************************************************
 * EfficentMigrationManager Implementation                *  
 *********************************************************/



EfficentMigrationManager::EfficentMigrationManager(){
	verbose = MyTopology::verbose_mig; 	
}

EfficentMigrationManager::~EfficentMigrationManager(){

}

bool EfficentMigrationManager::pre_classify(
						Packet* p, Handler* h, Node* n){
						
	hdr_ip* iph = hdr_ip::access(p); 

	auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;	
	
	auto n_addr = n->address(); 

	Tunnel_Point tp = rules[n_addr][p_dst];

	if (tp == Tunnel_Point::Tunnel_In) {
		// return tunnel_packet_in(td, p, n); 
	} else if (tp == Tunnel_Point::Tunnel_Out) {
		// return tunnel_packet_out(td, p, n); 
	} else if (tp == Tunnel_Point::Tunnel_From) {
		// return handle_packet_from(td, p, h, n);
	} else if (tp == Tunnel_Point::Tunnel_To) {
		// return handle_packet_to(td, p, h, n); 
	}	

	return true; 
}


void EfficentMigrationManager::deactivate_tunnel(int uid){

}


void EfficentMigrationManager::add_tunnel(tunnel_data tunnel){
	
	auto t_in = tunnel.in->address(); 
	auto t_out = tunnel.out->address(); 
	auto t_from = tunnel.from->address(); 
	auto t_to = tunnel.to->address(); 

	rules[t_in][t_from] = Tunnel_Point::Tunnel_In;
	rules[t_out][t_out] = Tunnel_Point::Tunnel_Out;
	rules[t_out][t_from] = Tunnel_Point::Tunnel_Out;
	rules[t_from][0] = Tunnel_Point::Tunnel_From; 
	rules[t_to][t_to] = Tunnel_Point::Tunnel_To; 

}
