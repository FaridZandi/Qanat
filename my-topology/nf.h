#ifndef nf_h
#define nf_h

#include "object.h"
#include <string>
#include <map>

class Packet; 
class Handler; 
class TopoNode; 
class PacketQueue; 

class NF : public Handler {
public: 
    NF(TopoNode* toponode, int chain_pos); 
    virtual ~NF(); 

    /**
     * @brief Entry point for the network function. All packets
     * should go through this function.  
     * 
     * @param p The packet. 
     * @param h The event handler. 
     * 
     * @return true if the packet should continue it's normal
     * path at the same moment of simulation. 
     * @return false if the packet should be ignored by the rest 
     * of the routing logic. Futher handling of the packet is done
     * by this function. (it might schedule some event for future).
     */
    virtual bool recv(Packet* p, Handler* h) = 0;


    /**
     * @brief Event handler function.
     * 
     * The NF can possible schedule events for the future.
     * Those event can be handled by itselft. 
     * 
     * @param event The event (packet) to be handled. 
     */
    virtual void handle(Event* event) = 0;


    /**
     * @brief Get the position of this NF in the chain. 
     * 
     * @return int The position in the chain.
     */
    int get_chain_pos(); 

    /**
     * @brief decrement the chain_pos value. When an NF
     * is removed from the earlier positions in the chain,
     * this function is called on all the subsequent nfs 
     * in the chain.
     */
    void decrement_chain_pos();

    /**
     * @brief prints some info about this NF. To be extended
     * by each NF type. 
     */
    virtual void print_info();

    /**
     * @brief Get the type of this NF. 
     * 
     * @return std::string The type of the NF. 
     */
    virtual std::string get_type() = 0; 

protected: 

    void send(Event* e);

    TopoNode* toponode_; 
    int chain_pos_;
};


class Monitor : public NF {
public:
    Monitor(TopoNode* toponode, int chain_pos); 

    virtual ~Monitor(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event);

    virtual std::string get_type(); 
    
    virtual void print_info(); 

private:

    int packet_count;
    std::map<int, int> dst_counter; 
    std::map<int, int> src_counter; 

};



class Buffer : public NF {
public:
    Buffer(TopoNode* toponode, int chain_pos, int size); 
    
    virtual ~Buffer(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event){};

    void start_buffering();

    void stop_buffering();

    virtual std::string get_type(); 

    virtual void print_info(); 

private:
    int size_; 

    bool buffering; 
    PacketQueue* pq;
};


class RateLimiterNF : public NF {
public:
    RateLimiterNF(TopoNode* toponode, int chain_pos, int rate); 
    
    virtual ~RateLimiterNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event);

    virtual std::string get_type(); 

    virtual void print_info(); 

private:
    void send_and_sched();
    double get_interval(); 

    bool busy_; 
    int rate_; 

    PacketQueue* pq;
};


class DelayerNF : public NF {
public:
    DelayerNF(TopoNode* toponode, int chain_pos, double delay); 
    
    virtual ~DelayerNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event);

    virtual std::string get_type(); 

    virtual void print_info(); 
    
private:
    double delay; 
};

class TunnelManagerNF : public NF {
public: 
    TunnelManagerNF(TopoNode* toponode, int chain_pos); 
    
    virtual ~TunnelManagerNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event){};

    virtual std::string get_type(); 

    virtual void print_info(); 
};


#endif