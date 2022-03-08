#include "mig_manager.h"
#include "node.h"
#include "packet.h"
#include <iostream>

int MigrationManager::tunnel_uid_counter = 0;

MigrationManager::MigrationManager(){
	tunnels = new tunnel_data[tunnel_count]; 
	for (int i = 0; i < tunnel_count; i++){
		tunnels[i].valid = false;
	}
}

MigrationManager::~MigrationManager(){
	delete[] tunnels; 
}


int MigrationManager::activate_tunnel(Node* in, Node* out, 
                                 Node* src, Node* dst,
                                 Node* dst_new){
    tunnel_data td; 

    td.valid = true;
    td.in = in; 
    td.out = out; 
    td.src = src; 
    td.dst = dst; 
    td.dst_new = dst_new; 
    td.uid = MigrationManager::tunnel_uid_counter ++;

    add_tunnel(td);                             

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


void MigrationManager::add_tunnel(tunnel_data tunnel){
	for(int i = 0; i < MigrationManager::tunnel_count; i++){
		if (not tunnels[i].valid){
			tunnels[i] = tunnel;
            return; 
        }
	}
}

void MigrationManager::log_packet(Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 

	std::cout << "pre_classify "; 
    std::cout << iph->src_.addr_ << " to " << iph->dst_.addr_;
    std::cout << std::endl; 
}

void MigrationManager::log_tunnel(tunnel_data td, 
								  Tunnel_Point tp, 
								  Packet* p){
	if (tp == Tunnel_Point::Tunnel_None){
		return; 
	}

	std::cout << "[tunnel " << td.uid << "] "; 
	
	if (tp == Tunnel_Point::Tunnel_In) {
		std::cout << "tunnel in detected"; 
	} else if (tp == Tunnel_Point::Tunnel_Out) {
		std::cout << "tunnel out detected"; 
	} else if (tp == Tunnel_Point::Tunnel_Dst) {
		std::cout << "tunnel dst detected"; 
	} else if (tp == Tunnel_Point::Tunnel_Dst_New) {
		std::cout << "tunnel dst new detected"; 
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
	auto t_src = td.src->address(); 
	auto t_dst = td.dst->address(); 
	auto t_dst_new = td.dst_new->address(); 

	auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;	
	
	auto p_src_orig = iph->src_orig.addr_;
	auto p_dst_orig = iph->dst_orig.addr_;

	auto n_addr = n->address(); 


	if (n_addr == t_in){ 
		if (p_src == t_src and p_dst == t_dst){
			return Tunnel_Point::Tunnel_In; 
		}
	} else if (n_addr == t_out){
		if (p_src == t_in and p_dst == t_out){
			if (p_src_orig == t_src and p_dst_orig == t_dst) {
				return Tunnel_Point::Tunnel_Out; 
			}
		}		
	} else if (n_addr == t_dst){
		if (p_src == t_src and p_dst == t_dst){
			return Tunnel_Point::Tunnel_Dst;
		} else if (p_src == t_dst and p_dst == t_src){
			return Tunnel_Point::Tunnel_Dst;
		}
	} else if (n_addr == t_dst_new){
		if (p_src == t_src and p_dst == t_dst_new){
			return Tunnel_Point::Tunnel_Dst_New;
		}
	} 
	return Tunnel_Point::Tunnel_None; 
}

Direction MigrationManager::packet_dir(tunnel_data td, 
									   Packet* p){
	hdr_ip* iph = hdr_ip::access(p); 

	auto t_in = td.in->address(); 
	auto t_out = td.out->address(); 
	auto t_src = td.src->address();
	auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;

	// TODO: make this independent of the source address
	// Then we can support a tunnel with any source address. 
	if (p_src == t_src){
		return Direction::Incoming; 
	} else if (p_src == t_in){
		return Direction::Incoming; 
	} else if (p_dst == t_src){
		return Direction::Outgoing; 
	} 

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
    
	log_packet(p);

    for(int i = 0; i < MigrationManager::tunnel_count; i++){ 
		auto td = tunnels[i]; 
		Tunnel_Point tp = packet_match(td, p, n); 
		
		// log_tunnel(td, tp, p);

		if (tp == Tunnel_Point::Tunnel_None) {
			continue; 
		} else if (tp == Tunnel_Point::Tunnel_In) {
			return tunnel_packet_in(td, p, n); 
		} else if (tp == Tunnel_Point::Tunnel_Out) {
			return tunnel_packet_out(td, p, n); 
		} else if (tp == Tunnel_Point::Tunnel_Dst) {
			return handle_packet_at_dst(td, p, h, n);
		} else if (tp == Tunnel_Point::Tunnel_Dst_New) {
			return handle_packet_at_dst_new(td, p, h, n); 
		}

	}
    return true; 
}


bool MigrationManager::tunnel_packet_in(tunnel_data td, 
								  Packet*p, Node* n){			  

	// The packet should be tunneled to the tunnel_out
	// point of this tunnel. The original source and 
	// destination of this packet are recorded in temp
	// variables in their ip header. 

	hdr_ip* iph = hdr_ip::access(p); 	
	iph->dst_orig = iph->dst_;
	iph->src_orig = iph->src_;
	iph->dst_.addr_ = td.out->address();
	iph->src_.addr_ = n->address();
	iph->prio_ = 15;

    return true; 	 
}

bool MigrationManager::tunnel_packet_out(tunnel_data td, 
								   Packet*p, Node* n){

	// Packet has reached the end of the tunnel. 
	// Original source will be recovered, but instead
	// of the original destination, the new destination
	// will be assigned to the packet. 

	hdr_ip* iph = hdr_ip::access(p); 	
	iph->src_ = iph->src_orig;
	iph->dst_.addr_ = td.dst_new->address(); 
	iph->prio_ = 0;

    return true; 
}

bool MigrationManager::handle_packet_at_dst(tunnel_data td, 
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
	// destination to deliver it to the traffic source.   

	hdr_ip* iph = hdr_ip::access(p);
	Direction dir = packet_dir(td, p);

    if(iph->agent_tunnel_flag){
        return true; // don't do anything 
    } else {
        if (dir == Direction::Outgoing){ 
            iph->src_.addr_ = td.dst_new->address(); 
            iph->agent_tunnel_flag = false;
            auto cls = td.dst_new->get_classifier();
            cls->recv(p, h);
            return false;  

        } else if (dir == Direction::Incoming){ 
            iph->dst_.addr_ = td.dst_new->address(); 
            iph->prio_ = 15;
            return true;         
        }
    }
}

bool MigrationManager::handle_packet_at_dst_new(tunnel_data td, 
                                          Packet*p, Handler* h, 
                                          Node* n){

	// Turn on the tunnel flag, to differentiate it 
	// from the traffic that had directly been sent
	// to the dstination from the srouce, but wasn't
	// tunneled in the gateway level, since the packet
	// had already passed the gateway when the tunnel 
	// was established. 

    hdr_ip* iph = hdr_ip::access(p); 	
    iph->dst_.addr_ = td.dst->address(); 
    iph->agent_tunnel_flag = true;
    
	td.dst->get_classifier()->recv(p, h); 

    return false; 
}
