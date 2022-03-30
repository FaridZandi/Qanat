#include "nf.h"
#include "node.h"
#include "topo_node.h"
#include "my_topology.h"
#include "mig_manager.h"
#include <iostream>
#include <sstream>

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



/**********************************************************
 * StatefulNF Implementation                              *  
 *********************************************************/

StatefulNF::StatefulNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {

    is_loading = false; 
    is_recording = false; 
}

StatefulNF::~StatefulNF(){

}

bool StatefulNF::recv(Packet* p, Handler* h){
    load_state(p); 
    return true; 
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

void StatefulNF::increment_key(std::string key){
    
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
}


void StatefulNF::load_state(Packet* p){
    if (is_loading){
        hdr_ip* iph = hdr_ip::access(p); 

        // std::cout << iph->state_dst << " " << toponode_->me->address() << std::endl; 

        if (iph->state_dst == toponode_->me->address()){
            state[std::string(iph->key)] = std::string(iph->value); 

            std::cout << "Node " << this->toponode_->me->address(); 
            std::cout << " loaded " << iph->key << ": " << state[iph->key];
            std::cout << " from packet" << std::endl;

            delete iph->key;
            delete iph->value; 
            iph->state_dst = -1; 
        }
    }
}

void StatefulNF::record_state(std::string key, Packet* p){

    if (is_recording){
        hdr_ip* iph = hdr_ip::access(p); 

        iph->state_dst = this->toponode_->peer->address();

        iph->key = new char[key.length() + 1];
        memcpy(iph->key, key.c_str(), key.length() + 1); 

        auto value = state[key];
        iph->value = new char[value.length() + 1];
        memcpy(iph->value, value.c_str(), value.length() + 1);

        std::cout << "Node " << this->toponode_->me->address(); 
        std::cout << " stored " << iph->key << ": " << iph->value;
        std::cout << " to packet" << std::endl;
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
    StatefulNF::recv(p, h); 

    if(verbose){
        std::cout << "[monitr] "; 
        std::cout << toponode_->me->address();  
        std::cout << " recved packet here" << std::endl;
    }

    hdr_ip* iph = hdr_ip::access(p); 
    auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;

    increment_key("packet_count");
    auto five_tuple = get_five_tuple(p);
    increment_key(five_tuple); 

    record_state(five_tuple, p);

    return true; 
}

void Monitor::handle(Event* event){
    send(event); 
}

std::string Monitor::get_type(){
    return "monitor"; 
}

void Monitor::print_info(){
    std::cout << "monitor on node " ;
    std::cout << toponode_->me->address();
    std::cout << ", current count is: "; 
    std::cout << state["packet_count"]; 
    std::cout << std::endl;  
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
    if(verbose){
        std::cout << "[buffer] "; 
        std::cout << toponode_->me->address();   
    }

    if(buffering){
        if(pq->length() == size_){
            // TODO: drop the packet (free the memory, etc.) 
            if(verbose){
                std::cout << " Buffer is full. Dropping packet. ";
                std::cout << std::endl;
            }
        } else {
            pq->enque(p);

            if(verbose){
                std::cout << " Buffering the packet. New Q length:"; 
                std::cout << pq->length() << std::endl;
            }
        }
        return false;
    } else {        
        if(verbose){
            std::cout << " Letting the packet pass through.";
            std::cout <<  std::endl;
        }   
        return true; 
    }
}

void Buffer::start_buffering(){
    buffering = true;
}

void Buffer::stop_buffering(){
    while (pq->length() > 0){
        if(verbose){
            std::cout << "[buffer] "; 
            std::cout << toponode_->me->address();   
            std::cout << " releasing a packet here. " << std::endl;
        }

        send(pq->deque());
    } 

    buffering = false; 
}

std::string Buffer::get_type(){
    return "buffer"; 
}

void Buffer::print_info(){
    std::cout << "buffer on node " ;
    std::cout << this->toponode_->me->address();
    std::cout << " with size "; 
    std::cout << this->size_;
    std::cout << std::endl;  
}

/**********************************************************
 * RateLimiterNF's Implementation                                 *  
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
    if(verbose){
        std::cout << "[ratlim] ";
        std::cout << toponode_->me->address();
    }

    if(busy_){
        pq->enque(p);

        if(verbose){
            std::cout << " Queuing the packet. New Q length:"; 
            std::cout << pq->length() << std::endl;
        }

        return false; 
    } else {
        if(verbose){
            std::cout << " Letting the packet pass through.";
            std::cout << std::endl; 
        }
        
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
    if(verbose){
        std::cout << "[ratlim] ";
        std::cout << toponode_->me->address();
        std::cout << " DeQ packet. New Q length:"; 
        std::cout << pq->length() - 1 << std::endl;
    }

    send(pq->deque()); 

    Event* e = new Event;
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, get_interval());
}

std::string RateLimiterNF::get_type(){
    return "rate_limiter"; 
}

void RateLimiterNF::print_info(){
    std::cout << "rate_limiter on node " ;
    std::cout << this->toponode_->me->address();
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

    if(verbose){
        std::cout << "[delayr] ";
        std::cout << toponode_->me->address(); 
        std::cout << " recved a packet here." << std::endl;
    }
    
    Scheduler::instance().schedule(this, p, delay);

    return false; 
}

void DelayerNF::handle(Event* event){
    send(event); 
}

std::string DelayerNF::get_type(){
    return "delayer"; 
}

void DelayerNF::print_info(){
    std::cout << "delayer on node " ;
    std::cout << this->toponode_->me->address();
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

    if(verbose){   
        std::cout << "[tunnel] ";
        std::cout << toponode_->me->address(); 
        std::cout << " recved a packet here." << std::endl;
    }

    auto& topo = MyTopology::instance(); 
    auto& mig_manager = topo.mig_manager(); 
    
    bool bypass_processing = mig_manager.bypass_processing(
        p, h, toponode_->me
    );

    bool ret =  mig_manager.pre_classify(
        p, h, toponode_->me
    );

    if (not ret){
        return false;
    }

    if (bypass_processing){
        toponode_->me->get_classifier()->recv2(p, h); 
        return false; 
    }

    return true;
}

std::string TunnelManagerNF::get_type(){
    return "tunnel_manager"; 
}

void TunnelManagerNF::print_info(){
    std::cout << "tunnel_manager on node " ;
    std::cout << this->toponode_->me->address();
    std::cout << std::endl;  
}
