#include <iostream>
#include "network_func.h"


class StorageNF: public NF{
public: 

    StorageNF(TopoNode* toponode, int chain_pos); 
    
    virtual ~StorageNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event);

    virtual std::string get_type(); 

    virtual void print_info(); 

    std::string increment_key(std::string key);

    void process_request(Packet* p);

private: 
    std::map<std::string, std::string> state; 

    void send_and_sched();
    double get_interval(); 

    bool busy_; 
    int rate_; 

    PacketQueue* pq;

};