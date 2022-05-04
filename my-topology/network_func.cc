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

NF::NF(TopoNode* toponode, 
       int chain_pos) : toponode_(toponode),
                        chain_pos_(chain_pos) {
                            
    verbose = false;
}

NF::~NF(){

}

void NF::send(Event* e){
    Packet* p = (Packet*) e;

    toponode_->process_packet(
        p, p->handler_, chain_pos_ + 1
    );
}

bool NF::is_stateful(){
    return false; 
}

int NF::get_chain_pos(){
    return chain_pos_; 
}

void NF::decrement_chain_pos(){
    chain_pos_ =- 1; 
}

void NF::print_info(){
    std::cout << get_type() << std::endl; 
}

bool NF::should_ignore(Packet* p){
    hdr_ip* iph = hdr_ip::access(p); 
    hdr_tcp* tcph = hdr_tcp::access(p); 

    if (iph->traffic_class == 2){
        return true; 
    }

    return false; 
}

bool NF::log_packet(std::string message, int arg){
    if(verbose){    
        print_time();

        std::cout << "[";
        std::cout << get_type();
        std::cout << "] ";


        std::cout << "["; 
        std::cout << "node " << toponode_->node->address();
        std::cout << "(" << toponode_->uid << ")";
        std::cout << "] "; 

        std::cout << message; 

        if (arg != -1){
            std::cout << " " << arg; 
        }
        
        std::cout << std::endl;
    }
}


/**********************************************************
 * StatefulNF Implementation                              *  
 *********************************************************/

StatefulNF::StatefulNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {
    access_mode = REMOTE; 
}

StatefulNF::~StatefulNF(){

}

std::string StatefulNF::get_five_tuple(Packet* p){
    std::stringstream five_tuple;
    hdr_ip* iph = hdr_ip::access(p); 

    five_tuple << iph->src_.addr_ << "-";
    five_tuple << iph->src_.port_ << "-";
    five_tuple << iph->dst_.addr_ << "-";
    five_tuple << iph->dst_.port_;

    // where is the protocol?  
    // five_tuple << protocol;

    return five_tuple.str();
}

bool StatefulNF::increment_key(Packet* p, std::string key){

    if (access_mode == LOCAL){
        std::string value; 
        if (state.find(key) == state.end()) {
            value = "0";
        } else {
            value = state[key];
        }   

        int count = std::stoi(value) + 1;
        state[key] = to_string(count);
        if (verbose){
            std::cout << "increment " << key << " to " << state[key] << std::endl; 
        }
        return true; 

    } else if (access_mode == REMOTE) {
        auto& topo = MyTopology::instance(); 
        auto storage_node = topo.storage_node; 

        hdr_ip* iph = hdr_ip::access(p); 
        iph->prio_ = 15;
        
        iph->key = new char[key.length() + 1];
        memcpy(iph->key, key.c_str(), key.length() + 1); 


        add_to_path(p, toponode_->node->address());
        add_to_path(p, storage_node->address());

        return true; 
    }  

}

bool StatefulNF::is_stateful(){
    return true;
}

void StatefulNF::print_state(){
    for (auto const& x : state){
        std::cout << x.first  << ':' << x.second << std::endl;
    }
}

std::string StatefulNF::get_main_state(){
    return 0; 
}


void StatefulNF::add_to_path(Packet* p, int addr){
    log_packet("adding to path", addr);

	auto& topo = MyTopology::instance();
	hdr_ip* iph = hdr_ip::access(p); 
    // if (iph->gw_path_pointer == -1){
    //     log_packet("adding to path", iph->dst_.addr_);
    //     iph->gw_path_pointer = 0; 
    //     iph->gw_path[0] = iph->dst_.addr_;
    // } 
    iph->gw_path_pointer += 1; 
    iph->gw_path[iph->gw_path_pointer] = addr;
}


/**********************************************************
 * Monitor Implementation                                 *  
 *********************************************************/

Monitor::Monitor(TopoNode* toponode, int chain_pos) 
    : StatefulNF(toponode, chain_pos) {
        state["packet_count"] = "0"; 
}

Monitor::~Monitor(){

}

bool Monitor::recv(Packet* p, Handler* h){
    hdr_ip* iph = hdr_ip::access(p); 

    if (iph->is_storage_response){
        log_packet("storage response received");
        state[std::string(iph->key)] = std::string(iph->value); 
        iph->is_storage_response = false; 
        iph->prio_ = 0;

        delete iph->key;
        delete iph->value; 

        return true; 
    } else {
        log_packet("recved packet here");

        auto p_src = iph->src_.addr_;
        auto p_dst = iph->dst_.addr_;

        // increment_key(p, "packet_count");

        auto key = get_key(p);
        return increment_key(p, key); 
    }
}

void Monitor::handle(Event* event){
    send(event); 
}

std::string Monitor::get_type(){
    return "monitr"; 
}

void Monitor::print_info(){
    std::cout << "monitor on node " ;
    std::cout << toponode_->node->address();
    std::cout << ", current count is: "; 
    std::cout << state["packet_count"]; 
    std::cout << std::endl;  
}

std::string Monitor::get_key(Packet* p){
    std::stringstream key;
    key << "packet_count-"; 
    key << toponode_->node->address(); 
    // if (p != NULL) {
        // hdr_ip* iph = hdr_ip::access(p);
        // key << "-" << iph->gw_path[0]; 
    // }
    return key.str();
}



std::string Monitor::get_main_state(){
    // return std::to_string(state.size());
    return state[get_key(NULL)];
}


/**********************************************************
 * Buffer Implementation                                  *  
 *********************************************************/


Buffer::Buffer(TopoNode* toponode, int chain_pos, int size) 
    : NF(toponode, chain_pos) {

    pq = new PacketQueue;
    buffering = false; 
    this->size_ = size; 
}

Buffer::~Buffer(){
    delete pq; 
}

bool Buffer::recv(Packet* p, Handler* h){
    hdr_ip* iph = hdr_ip::access(p);

    log_packet("recved packet here with class: ", iph->traffic_class);

    if (should_ignore(p)){
        return true;
    }

    if(buffering){
        if(pq->length() == size_){
            // TODO: drop the packet (free the memory, etc.) 
            log_packet("Buffer is full. Dropping packet.");
        } else {
            pq->enque(p);
            log_packet("Buffering the packet. New Q length:", pq->length());
        }
        return false;
    } else {     
        log_packet("Letting the packet pass through.");   
        return true; 
    }
}

void Buffer::start_buffering(){
    buffering = true;
}

int Buffer::get_buffer_size(){
    return pq->length(); 
}

void Buffer::stop_buffering(){
    buffering = false; 

    while (pq->length() > 0){
        log_packet("releasing a packet here.");
        send(pq->deque());
    } 

}

std::string Buffer::get_type(){
    return "buffer"; 
}

void Buffer::print_info(){
    std::cout << "buffer on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << "(" << this->toponode_->uid << ")";
    std::cout << " with size "; 
    std::cout << this->size_;
    std::cout << std::endl;  
}


/**********************************************************
 * SelectiveBuffer's Implementation                       *  
 *********************************************************/


SelectiveBuffer::SelectiveBuffer(TopoNode* toponode, int chain_pos, int size) 
    : Buffer(toponode, chain_pos, size) {
        buffer_packets_from = -1; 
}

SelectiveBuffer::~SelectiveBuffer(){

}



bool SelectiveBuffer::recv(Packet* p, Handler* h){
    hdr_ip* iph = hdr_ip::access(p);

    log_packet("recved packet here with class: ", iph->traffic_class);

    if (should_ignore(p)){
        return true;
    }

    if (not buffering){
        return true; 
    }

    // only care about packets from a certain prev hop
    if (iph->prev_hop != buffer_packets_from){
        return true; 
    }
    
    return Buffer::recv(p, h); 
}


std::string SelectiveBuffer::get_type(){
    return "selbuf"; 
}

void SelectiveBuffer::print_info(){
    std::cout << "selective buffer on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << "(" << this->toponode_->uid << ")";
    std::cout << " with size "; 
    std::cout << this->size_;
    std::cout << std::endl;  
}



/**********************************************************
 * RateLimiterNF's Implementation                         *  
 *********************************************************/

RateLimiterNF::RateLimiterNF(TopoNode* toponode, int chain_pos, int rate) 
    : NF(toponode, chain_pos) {

    busy_ = false;
    pq = new PacketQueue;
    this->rate_ = rate; 
}

RateLimiterNF::~RateLimiterNF(){
    delete pq; 
}

double RateLimiterNF::get_interval(){
    double wait = 1.0 / rate_;
    return wait;  
}

bool RateLimiterNF::recv(Packet* p, Handler* h){

    if(busy_){
        pq->enque(p);
        log_packet("Queuing the packet. New Q length:", pq->length());
        return false; 
    } else {

        log_packet("Letting the packet pass through.");

        busy_ = true; 
        Event* e = new Event; 
        auto& sched = Scheduler::instance();
        sched.schedule(this, e, get_interval());
        return true; 
    }
}

void RateLimiterNF::handle(Event* event){
    if (pq->length() == 0){
        busy_ = false;  
    } else {
        send_and_sched(); 
        // busy should be true at this point, 
        // but just in case: 
        busy_ = true; 
    }
}

void RateLimiterNF::send_and_sched(){
    log_packet("DeQ packet. New Q length:", pq->length() - 1);

    send(pq->deque()); 

    Event* e = new Event;
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, get_interval());
}

std::string RateLimiterNF::get_type(){
    return "ratlim"; 
}

void RateLimiterNF::print_info(){
    std::cout << "rate_limiter on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << ", rate is: "; 
    std::cout << this->rate_;
    std::cout << std::endl;  
}

/**********************************************************
 * DelayerNF's Implementation                                 *  
 *********************************************************/


DelayerNF::DelayerNF(TopoNode* toponode, int chain_pos, double delay) 
    : NF(toponode, chain_pos) {
    this->delay = delay; 
}

DelayerNF::~DelayerNF(){

}

bool DelayerNF::recv(Packet* p, Handler* h){

    log_packet("recved a packet here.");
    
    Scheduler::instance().schedule(this, p, delay);

    return false; 
}

void DelayerNF::handle(Event* event){
    send(event); 
}

std::string DelayerNF::get_type(){
    return "delayr"; 
}

void DelayerNF::print_info(){
    std::cout << "delayer on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << ", delay is: "; 
    std::cout << this->delay;
    std::cout << std::endl;  
}

/**********************************************************
 * TunnelManagerNF's Implementation                                 *  
 *********************************************************/

TunnelManagerNF::TunnelManagerNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {
}

TunnelManagerNF::~TunnelManagerNF(){

}

bool TunnelManagerNF::recv(Packet* p, Handler* h){
    log_packet("recved a packet here.");

    auto& topo = MyTopology::instance(); 
    auto& mig_manager = topo.mig_manager(); 
    
    return mig_manager.pre_classify(p, h, toponode_->node);
}

std::string TunnelManagerNF::get_type(){
    return "tnlmng"; 
}

void TunnelManagerNF::print_info(){
    std::cout << "tunnel_manager on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << std::endl;  
}



/**********************************************************
 * RouterNF's Implementation                                 *  
 *********************************************************/

RouterNF::RouterNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {
}

RouterNF::~RouterNF(){

}

bool RouterNF::recv(Packet* p, Handler* h){
    log_packet("recved a packet here.");

    hdr_ip* iph = hdr_ip::access(p);
    
    // std::cout << "src: " << iph->src_.addr_ << " dst: " << iph->dst_.addr_ << std::endl; 
    // std::cout << "this node address: " << this->toponode_->node->address() << std::endl; 

    if (iph->gw_path_pointer != -1){
        
        auto ptr = iph->gw_path_pointer;
        iph->prev_hop = iph->dst_.addr_; 
        iph->dst_.addr_ = iph->gw_path[ptr]; 
        iph->gw_path_pointer --; 
    }

    // std::cout << "src: " << iph->src_.addr_ << " dst: " << iph->dst_.addr_ << std::endl; 
    // std::cout << "-----------------------" << std::endl; 

	// std::cout << "new packet dst: " << iph->dst_.addr_ << std::endl; 

	// std::cout << "packet path: "; 
	// for (int i = 0; i <= iph->gw_path_pointer; i++){
	// 	std::cout << iph->gw_path[i] << " "; 
	// }
	// std::cout << std::endl; 

    

    return true;
}

std::string RouterNF::get_type(){
    return "router"; 
}

void RouterNF::print_info(){
    std::cout << "router on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << std::endl;  
}


/**********************************************************
 * LastPacketNotifNF's Implementation                                 *  
 *********************************************************/

LastPacketNotifNF::LastPacketNotifNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {
}

LastPacketNotifNF::~LastPacketNotifNF(){

}

bool LastPacketNotifNF::recv(Packet* p, Handler* h){
    log_packet("recved a packet here.");
    return true; 
}

std::string LastPacketNotifNF::get_type(){
    return "lastpk"; 
}

void LastPacketNotifNF::print_info(){
    std::cout << "Last Packet Notifier on node " ;
    std::cout << this->toponode_->node->address();
    std::cout << std::endl;  
}
