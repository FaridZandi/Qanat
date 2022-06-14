#include "network_func.h"
#include "node.h"
#include "topo_node.h"
#include "my_topology.h"
#include "mig_manager.h"
#include "orchestrator.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "tcp-full.h"
#include <iomanip>
#include "utility.h"

NF::NF(TopoNode* toponode, 
       int chain_pos) : toponode_(toponode),
                        chain_pos_(chain_pos) {
                            
    verbose = MyTopology::verbose_nf;
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

MigState NF::get_node_state(){
    auto& orch = BaseOrchestrator::instance(); 
    auto node_state = orch.get_mig_state(toponode_->node); 
    return node_state; 
}

/**********************************************************
 * StatefulNF Implementation                              *  
 *********************************************************/

StatefulNF::StatefulNF(TopoNode* toponode, int chain_pos) 
    : NF(toponode, chain_pos) {

    is_loading = false; 
    is_recording = false; 

    total_ooo_packets = 0;
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

        // std::cout << iph->state_dst << " " << toponode_->node->address() << std::endl; 

        if (iph->state_dst == toponode_->node->address()){
            state[std::string(iph->key)] = std::string(iph->value); 

            std::cout << "Node " << this->toponode_->node->address(); 
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

        auto& topo = MyTopology::instance();
        auto peer = topo.get_peer(this->toponode_->node); 

        iph->state_dst = peer->address();

        iph->key = new char[key.length() + 1];
        memcpy(iph->key, key.c_str(), key.length() + 1); 

        auto value = state[key];
        iph->value = new char[value.length() + 1];
        memcpy(iph->value, value.c_str(), value.length() + 1);

        std::cout << "Node " << this->toponode_->node->address(); 
        std::cout << " stored " << iph->key << ": " << iph->value;
        std::cout << " to packet" << std::endl;
    }
}


void StatefulNF::update_highest_seq(Packet* p){
    hdr_ip* iph = hdr_ip::access(p); 
    hdr_tcp* tcph = hdr_tcp::access(p);

    auto fid = iph->fid_; 
    auto seqno = tcph->seqno();
    auto reason = tcph->reason();

    if(fid == 0){
        return;
    }

    if(highest_seq.find(fid) == highest_seq.end()){
        highest_seq[fid] = 0; 
        ooo_packets[fid] = 0; 
    }

    if(seqno < highest_seq[fid] and reason == 0){
        // std::cout << "on node " << toponode_->node->address() << ": ";
        // std::cout << "highest_seq[" << fid << "] = " << highest_seq[fid];
        // std::cout << " seqno = " << seqno << std::endl;
        // std::cout << "reason = " << tcph->reason() << std::endl;
        
        ooo_packets[fid] ++; 
        total_ooo_packets ++;
    }

    highest_seq[fid] = std::max(highest_seq[fid], seqno);
}

int StatefulNF::get_ooo_packet_count(){
    return total_ooo_packets; 
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
        // std::cout << "initializing packet count on node " << toponode_->node->address() << std::endl; 
        state["packet_count"] = "0"; 
}

Monitor::~Monitor(){

}

bool Monitor::recv(Packet* p, Handler* h){
    if (should_ignore(p)){
        return true; 
    }

    // if the node is in the normal mode, packet is processed
    // otherwise, just let the packet pass through.
    if (get_node_state() != MigState::Normal){
        return true; 
    }

    log_packet("recved packet here");

    hdr_ip* iph = hdr_ip::access(p); 
    auto p_src = iph->src_.addr_;
	auto p_dst = iph->dst_.addr_;

    increment_key("packet_count");
    auto five_tuple = get_five_tuple(p);
    increment_key(five_tuple); 

    record_state(five_tuple, p);

    update_highest_seq(p); 

    return true; 
}

void Monitor::handle(Event* event){
    send(event); 
}

std::string Monitor::get_type(){
    return "monitr"; 
}

int Monitor::get_packet_count(){
    return std::stoi(state["packet_count"]);
}

void Monitor::print_info(){
    std::cout << "monitor on node " ;
    std::cout << toponode_->node->address();
    std::cout << ", PCount is: "; 
    std::cout << state["packet_count"]; 
    std::cout << std::endl;  

    for (auto flow : ooo_packets){
        std::cout << flow.first << ": " << flow.second << std::endl; 
    }
}



/*********************************************************
* Buffer Implementation                                  *  
*********************************************************/


Buffer::Buffer(TopoNode* toponode, int chain_pos, int size) 
    : NF(toponode, chain_pos) {
    pq = new PacketQueue;

    buffering_ = false; 
    busy_ = false; 
    size_ = size; 
    rate_ = 868055;
}

Buffer::~Buffer(){
    delete pq;
}


bool Buffer::recv(Packet* p, Handler* h){
    hdr_ip* iph = hdr_ip::access(p);
    log_packet("recved packet with class: ", iph->traffic_class);

    if (should_ignore(p)){
        return true;
    }   

    if(buffering_ or busy_){
        auto& topo = MyTopology::instance();
        auto peer = topo.get_peer(toponode_->node);

        if (pq->length() < size_){
            pq->enque(p);
            iph->time_enter_buffer = Scheduler::instance().clock();
            log_packet("The new queue size is: ", pq->length());
        } else {
            log_packet("Queue is full. dropping the packet ");
        }

        return false;
    } else {     
        log_packet("Letting the packet pass through.");  
        busy_ = true; 
        sched_next_send(); 
        return true; 
    }
}

double Buffer::get_interval(){
    double wait = 1.0 / rate_;
    return wait;  
}

void Buffer::sched_next_send(){
    Event* e = new Event; 
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, get_interval());
}

void Buffer::handle(Event* event){
    busy_ = false; 
    send_if_possible();
}

void Buffer::start_buffering(){
    buffering_ = true;
}

void Buffer::stop_buffering(){
    buffering_ = false; 
    send_if_possible(); 
}

void Buffer::send_if_possible(){
    if (busy_){
        return;
    }

    auto now = Scheduler::instance().clock();

    if (pq->length() > 0){
        busy_ = true; 
        sched_next_send(); 

        Packet* p = pq->deque();        
        hdr_ip* iph = hdr_ip::access(p);
        iph->time_buffered += (now - iph->time_enter_buffer);
        iph->time_enter_buffer = -1;
        send(p); 
    } 
}

void Buffer::set_rate(int rate){
    rate_ = rate; 
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


/*********************************************************
* PriorityBuffer Implementation                          *  
*********************************************************/


PriorityBuffer::PriorityBuffer(TopoNode* toponode, int chain_pos, int size) 
    : NF(toponode, chain_pos) {

    lp_q = new PacketQueue;
    hp_q = new PacketQueue;

    buffering_ = false; 
    busy_ = false; 
    size_ = size; 
    rate_ = 868055;
}

PriorityBuffer::~PriorityBuffer(){
    delete lp_q; 
    delete hp_q; 
}


bool PriorityBuffer::recv(Packet* p, Handler* h){
    hdr_ip* iph = hdr_ip::access(p);
    log_packet("recved packet with class: ", iph->traffic_class);

    if (should_ignore(p)){
        return true;
    }   

    if(buffering_ or busy_){
        int queue_number;
        PacketQueue* pq;

        auto& topo = MyTopology::instance();
        auto peer = topo.get_peer(toponode_->node);

        if (iph->prev_hop == peer->address()){
            // if the packet is coming from the peer, 
            // it means that it has been tunnelled, so it 
            // should be processed with high priority. 
            queue_number = 1;
            pq = hp_q;
        } else {
            // otherwise, the packet will be processed with 
            // a low priority. These are the packets coming from 
            // "above" in the tree. 
            queue_number = 2;
            pq = lp_q;
        }  

        if (pq->length() < size_){
            pq->enque(p);
            iph->time_enter_buffer = Scheduler::instance().clock();

            log_packet("Enqueued the packet in queue:", queue_number);
            log_packet("The new queue size is: ", pq->length());
        } else {
            log_packet("Queue is full. dropping the packet on: ", queue_number);
        }

        return false;
    } else {     
        log_packet("Letting the packet pass through.");  

        busy_ = true; 
        sched_next_send(); 

        return true; 
    }
}

double PriorityBuffer::get_interval(){
    double wait = 1.0 / rate_;
    return wait;  
}

void PriorityBuffer::sched_next_send(){
    Event* e = new Event; 
    auto& sched = Scheduler::instance();
    sched.schedule(this, e, get_interval());
}

void PriorityBuffer::handle(Event* event){
    busy_ = false; 
    send_if_possible();
}

void PriorityBuffer::start_buffering(){
    buffering_ = true;
}

void PriorityBuffer::stop_buffering(){
    buffering_ = false; 
    send_if_possible(); 
}

void PriorityBuffer::send_if_possible(){
    if (busy_){
        return;
    }

    auto now = Scheduler::instance().clock();

    if (hp_q->length() > 0){
        busy_ = true; 
        sched_next_send(); 

        Packet* p = hp_q->deque();        
        hdr_ip* iph = hdr_ip::access(p);
        iph->time_buffered += (now - iph->time_enter_buffer);
        iph->time_enter_buffer = -1;

        send(p); 

    } else if (lp_q->length() > 0) {
        busy_ = true; 
        sched_next_send(); 

        Packet* p = lp_q->deque();
        hdr_ip* iph = hdr_ip::access(p);
        iph->time_buffered += (now - iph->time_enter_buffer);
        iph->time_enter_buffer = -1;

        send(p); 
    } 
}

void PriorityBuffer::set_rate(int rate){
    rate_ = rate; 
}

std::string PriorityBuffer::get_type(){
    return "pribuf"; 
}

void PriorityBuffer::print_info(){
    std::cout << "priority buffer on node " ;
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
    
    if (should_ignore(p)){
        return true;
    }

    // if the node is in the normal mode, delay is applied
    // otherwise, just let the packet pass through.
    if (get_node_state() == MigState::Normal){
        Scheduler::instance().schedule(this, p, delay);
        return false; 
    } else {
        return true; 
    }
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

    if (should_ignore(p)){
        return true;
    }

    auto& topo = MyTopology::instance(); 
    auto& mig_manager = topo.mig_manager(); 

    auto ret = mig_manager.pre_classify(p, h, toponode_->node);

    if (! ret) return false; 

    // if top-down approach or random
    if (topo.orch_type == 2 or topo.orch_type == 3) {
        // if in the destination tree 
        if (topo.get_data(toponode_->node).which_tree == 1){
            if (get_node_state() != MigState::Normal and topo.is_migration_started){
                convert_path(p);
                auto this_node = toponode_->node;
                auto peer = topo.get_peer(this_node);
                add_to_path(p, peer->address());
                set_high_prio(p); 
                topo.inc_tunnelled_packets();
            }
        }
    }
    
    // anyhow, the priority tag is removed from the packet. 
    unset_high_prio(p); 

    return true;
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
    
    if (iph->gw_path_pointer != -1){
        auto ptr = iph->gw_path_pointer;
        iph->prev_hop = iph->dst_.addr_; 
        iph->dst_.addr_ = iph->gw_path[ptr]; 
        iph->gw_path_pointer --; 
    }


    if (verbose) {
        std::cout << "packet path: "; 
        for (int i = 0; i <= iph->gw_path_pointer; i++){
            std::cout << iph->gw_path[i] << " "; 
        }
        std::cout << "packet dst: " << iph->dst_.addr_;
        std::cout << std::endl; 
    }

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
