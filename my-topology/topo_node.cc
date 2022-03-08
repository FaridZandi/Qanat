#include "topo_node.h"


TopoNode::TopoNode(){
        peer = nullptr;
        me = nullptr;
        bypass_processing = false; 
        mode = OpMode::NoneMode;  
        layer_from_bottom = 0; 
}

TopoNode::~TopoNode(){}


void TopoNode::process_packet(Packet* p, Handler* h, 
                              int start_pos){
                                  
    for(uint current = start_pos; 
        current < nfs.size(); current++){

        if (not nfs[current]->recv(p, h)){
            return;
        } 
    }

    me->get_classifier()->recv2(p, h);
}

NF* TopoNode::add_nf(std::string type, double param){

    int current_length = nfs.size(); 

    if(type == "monitor"){
        Monitor* m = new Monitor(this, current_length);
        nfs.push_back(m); 
        return m; 

    } else if (type == "buffer"){
        int size = int(param); 
        Buffer* b = new Buffer(this, current_length, size);
        nfs.push_back(b); 
        return b;

    } else if (type == "rate_limiter"){
        int rate = int(param); 
        RateLimiterNF* rl = new RateLimiterNF(
            this, current_length, rate); 
        nfs.push_back(rl); 
        return rl;

    } else if (type == "delayer"){
        DelayerNF* d = new DelayerNF(
            this, current_length, param); 
        nfs.push_back(d); 
        return d; 

    } else if (type == "tunnel_manager"){
        TunnelManagerNF* tm = new TunnelManagerNF(
            this, current_length); 
        nfs.push_back(tm); 
        return tm; 
    }

    return nullptr;
}


NF* TopoNode::get_nf(std::string type, int occurence){
    int type_counter = 0; 
    for (auto& nf: nfs){
        if(nf->get_type() == type){
            type_counter ++; 
            if (type_counter == occurence){
                return nf; 
            }
        }
    }
    return nullptr; 
}

void TopoNode::remove_nf(std::string type){
    uint nfs_size = nfs.size(); 
    for (uint i = 0; i < nfs_size; i++){
        if (nfs[i]->get_type() == type){
            rm_nf_at_index(i); 
            i--; 
            nfs_size--; 
        }
    }
}

void TopoNode::rm_nf_at_index(int index){
    nfs.erase(nfs.begin() + index);
    for (uint i = index; i < nfs.size(); i++){
        nfs[i]->decrement_chain_pos(); 
    }
}

void TopoNode::print_nfs(){
    for (auto& nf: nfs){
        nf->print_info(); 
    }
}