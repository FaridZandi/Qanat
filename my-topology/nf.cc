#include "nf.h"
#include "node.h"
#include "topo_node.h"
#include "my_topology.h"
#include "mig_manager.h"
#include <iostream>

NF::NF(TopoNode* toponode, 
       int chain_pos) : toponode_(toponode),
                        chain_pos_(chain_pos) { }

NF::~NF(){

}

void NF::send(Event* e){
    Packet* p = (Packet*) e;

    toponode_->process_packet(
        p, p->handler_, chain_pos_ + 1
    );
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
 * Monitor Implementation                                 *  
 *********************************************************/


Monitor::Monitor(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {
    packet_count = 0; 
}

Monitor::~Monitor(){

}

bool Monitor::recv(Packet* p, Handler* h){
    std::cout << "[monitr] "; 
    std::cout << toponode_->me->address();  
    std::cout << " recved packet here" << std::endl;


    hdr_ip* iph = hdr_ip::access(p); 
    auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;


    packet_count++;
    dst_counter[p_dst]++; 
    src_counter[p_src]++; 

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
    std::cout << this->toponode_->me->address();
    std::cout << ", current count is: "; 
    std::cout << this->packet_count; 
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
    std::cout << "[buffer] "; 
    std::cout << toponode_->me->address();   

    if(buffering){
        if(pq->length() == size_){
            // TODO: drop the packet (free the memory, etc.) 
            std::cout << " Buffer is full. Dropping packet. ";
            std::cout << std::endl;
        } else {
            pq->enque(p); 
            std::cout << " Buffering the packet. New Q length:"; 
            std::cout << pq->length() << std::endl;
        }
        return false;
    } else {        
        std::cout << " Letting the packet pass through.";
        std::cout <<  std::endl;
        return true; 
    }
}

void Buffer::start_buffering(){
    buffering = true; 

    std::cout << "start buffering for ";
    std::cout << toponode_->me->address(); 
    std::cout << std::endl; 
}

void Buffer::stop_buffering(){
    std::cout << "stopped buffering for ";
    std::cout << toponode_->me->address(); 
    std::cout << std::endl; 
    
    while (pq->length() > 0){
        std::cout << "[buffer] "; 
        std::cout << toponode_->me->address();   
        std::cout << " releasing a packet here. " << std::endl;
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
    std::cout << "[ratlim] ";
    std::cout << toponode_->me->address();

    if(busy_){
        pq->enque(p); 
        std::cout << " Queuing the packet. New Q length:"; 
        std::cout << pq->length() << std::endl;
        return false; 
    } else {
        std::cout << " Letting the packet pass through.";
        std::cout << std::endl; 
        
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
    std::cout << "[ratlim] ";
    std::cout << toponode_->me->address();
    std::cout << " DeQ packet. New Q length:"; 
    std::cout << pq->length() - 1 << std::endl;

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
    std::cout << "[delayr] ";
    std::cout << toponode_->me->address(); 
    std::cout << " recved a packet here." << std::endl;
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
    std::cout << "[tunnel] ";
    std::cout << toponode_->me->address(); 
    std::cout << " recved a packet here." << std::endl;

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
