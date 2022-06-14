#include <iostream>
#include <iomanip>
#include "simulator.h"
#include "node.h"
#include "my_topology.h"

void print_time(){
    std::cout << "[";
    std::cout << std::setprecision(7);
    // std::cout << std::setw(5);
    std::cout << Scheduler::instance().clock();
    std::cout << "] ";
}

int get_random_transfer_size(int mean, int range_p){
    return mean; 
    

    
    int r = (100 - range_p) + std::rand() % (2 * range_p);
    int size = r * (mean / 100);
    return size; 
}


void convert_path(Packet* p){
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

void add_to_path(Packet* p, int addr){
	auto& topo = MyTopology::instance();
	hdr_ip* iph = hdr_ip::access(p); 

	iph->gw_path_pointer += 1; 
	iph->gw_path[iph->gw_path_pointer] = addr;
}

void set_high_prio(Packet* p){
    auto& topo = MyTopology::instance();
    hdr_ip* iph = hdr_ip::access(p);

    if (topo.prioritization_level > 0){
        iph->is_high_prio = 1;
    }
}

void unset_high_prio(Packet* p){
    hdr_ip* iph = hdr_ip::access(p);
    iph->is_high_prio = 0;
}


