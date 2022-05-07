#include "state_manager.h"
#include "network_func.h"
#include "node.h"
#include "topo_node.h"
#include "my_topology.h"
#include "mig_manager.h"
#include <iostream>
#include <sstream>
#include "tcp-full.h"
#include <iomanip>
#include "utility.h"

StorageNF::StorageNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {

    
    busy_ = false;
    pq = new PacketQueue;
    this->rate_ = MyTopology::remote_storage_rate; 

    verbose = false; 

}

StorageNF::~StorageNF(){

}

bool StorageNF::recv(Packet* p, Handler* h){
    // std::cout << "oh hello" << std::endl;

    log_packet("recved a packet here.");

    if(busy_){
        pq->enque(p);
        log_packet("Queuing the packet. New Q length:", pq->length());
        return false; 

    } else {
    
        process_request(p);

        busy_ = true; 
        Event* e = new Event; 
        auto& sched = Scheduler::instance();
        sched.schedule(this, e, get_interval());
        return true; 
    }

    return true; 
}

void StorageNF::handle(Event* event){
    if (pq->length() == 0){
        busy_ = false;  
    } else {
        send_and_sched(); 
        busy_ = true; 
    }
}


void StorageNF::send_and_sched(){
    log_packet("DeQ packet. New Q length:", pq->length() - 1);

    Packet* p = pq->deque();
    process_request(p);

    send(p); 

    Event* e = new Event;
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, get_interval());
}


double StorageNF::get_interval(){
    double wait = 1.0 / rate_;
    return wait;  
}

std::string StorageNF::get_type(){
    return "storag"; 
}

void StorageNF::print_info(){
    for (auto const &pair: state) {
        std::cout << "{" << pair.first << ": " << pair.second << "}\n";
    }
}

std::string StorageNF::increment_key(std::string key, int by){
    std::string value; 

    if (state.find(key) == state.end()) {
        value = "0";
    } else {
        value = state[key];
    }   

    int count = std::stoi(value) + by;
    state[key] = to_string(count);

    if (verbose){
        std::cout << "increment " << key << " to " << state[key] << std::endl; 
    }

    return to_string(count); 
}


int StorageNF::get_key_owner(std::string key){
    if (key_owner.find(key) == key_owner.end()){
        return -1; 
    } else {
        return key_owner[key];
    }
}


void StorageNF::process_request(Packet* p){
    log_packet("processing the packet");

    hdr_ip* iph = hdr_ip::access(p);
    std::string key = std::string(iph->key);

    if (access_mode == REMOTE or access_mode == CACHE){
        log_packet("serving storage op 1");
        std::string value = increment_key(key); 
        
        iph->value = new char[value.length() + 1];
        memcpy(iph->value, value.c_str(), value.length() + 1);
        
        // log_packet("setting the state owner to", iph->state_dst, true);
        // log_packet(key.c_str(),0, true);

        key_owner[key] = iph->state_dst; 

    } else if (access_mode == EVENTUAL) {
        log_packet("serving storage op 2");

        std::string diff_value = std::string(iph->value);
        log_packet("incrementing key by", std::stoi(diff_value));
        std::string value = increment_key(key, std::stoi(diff_value)); 
        
        delete[] iph->value; 

        iph->value = new char[value.length() + 1];
        memcpy(iph->value, value.c_str(), value.length() + 1);
    }
    
    iph->is_storage_response = true; 
}

