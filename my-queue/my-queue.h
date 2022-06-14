//
// Author:    Jae Chung
// File:      dtrr-queue.h
// Written:   07/19/99 (for ns-2.1b4a)
// Modifed:   10/14/01 (for ns-2.1b8a)
//

#include <string.h>
#include "queue.h"
#include "red.h"
#include "address.h"
#include <sstream>

class MyQueue : public Queue {
 public:
	MyQueue();

 protected:
	void enque(Packet*);
	Packet* deque();

	PacketQueue *q1_;   // low priority queue
	PacketQueue *q2_;   // high priority queue
	PacketQueue *q3_;   // very high priority queue
};



class MyREDQueue : public Queue {
 public:
	MyREDQueue();

 protected:
	void enque(Packet*);
	Packet* deque();

	PacketQueue *q1_;   // low priority queue
	PacketQueue *q2_;   // high priority queue
	PacketQueue *q3_;   // very high priority queue
};





class MyOtherREDQueue: public REDQueue {
public :
	MyOtherREDQueue();
	
protected: 

	void enque(Packet* p);
	Packet* deque();

	PacketQueue* q1_;
	PacketQueue* q2_;
	PacketQueue* q3_;

};


