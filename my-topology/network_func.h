#ifndef nf_h
#define nf_h

#include "object.h"
#include <string>
#include <map>

class Packet; 
class Handler; 
class TopoNode; 
class PacketQueue; 
class Node; 

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
     * Those event can be handled by itself. 
     * 
     * @param event The event (packet) to be handled. 
     */
    virtual void handle(Event* event) = 0;


    virtual bool is_stateful(); 

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

    bool should_ignore(Packet* p); 

    bool log_packet(std::string message, int arg = -1);

    void send(Event* e);

    TopoNode* toponode_; 
    int chain_pos_;

    bool verbose; 
};


enum ACCESS_MODE {
    LOCAL, 
    REMOTE, 
    EVENTUAL,
};

class StatefulNF : public NF {
public: 
    StatefulNF(TopoNode* toponode, int chain_pos); 

    virtual ~StatefulNF(); 

    virtual bool is_stateful(); 
    
    // utility functions 
    
    std::string get_five_tuple(Packet* p);

    bool increment_key(Packet* p, std::string key); 

    void print_state();

    virtual std::string get_main_state();

    void add_to_path(Packet* p, int addr);

protected: 
    std::map<std::string, std::string> state; 

    ACCESS_MODE access_mode; 
};



class Monitor : public StatefulNF {
public:
    Monitor(TopoNode* toponode, int chain_pos); 

    virtual ~Monitor(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event);

    virtual std::string get_type(); 
    
    virtual void print_info(); 

    std::string get_key(Packet* p);

    virtual std::string get_main_state();
};



class Buffer : public NF {
public:
    Buffer(TopoNode* toponode, int chain_pos, int size); 
    
    virtual ~Buffer(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event){};

    void start_buffering();

    void stop_buffering();

    int get_buffer_size(); 
    
    virtual std::string get_type(); 

    virtual void print_info(); 

protected:
    int size_; 

    bool buffering; 
    PacketQueue* pq;
};

class SelectiveBuffer: public Buffer {
public:
    SelectiveBuffer(TopoNode* toponode, int chain_pos, int size); 
    
    virtual ~SelectiveBuffer();

    virtual bool recv(Packet* p, Handler* h);

    virtual std::string get_type(); 

    virtual void print_info(); 

    int buffer_packets_from; 
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

class RouterNF : public NF {
public: 
    RouterNF(TopoNode* toponode, int chain_pos); 
    
    virtual ~RouterNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event){};

    virtual std::string get_type(); 

    virtual void print_info(); 
};



class LastPacketNotifNF : public NF {
public: 
    LastPacketNotifNF(TopoNode* toponode, int chain_pos); 
    
    virtual ~LastPacketNotifNF(); 

    virtual bool recv(Packet* p, Handler* h);

    virtual void handle(Event* event){};

    virtual std::string get_type(); 

    virtual void print_info(); 

    void (*finish_notify_callback) (Node*);
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