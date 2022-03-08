//
// Author:    Jae Chung
// File:      dtrr-queue.h
// Written:   07/19/99 (for ns-2.1b4a)
// Modifed:   10/14/01 (for ns-2.1b8a)
//

#include <string.h>
#include "queue.h"
#include "address.h"
#include <sstream>

class MyQueue : public Queue {
 public:
	MyQueue();

 protected:
	void enque(Packet*);
	Packet* deque();

	PacketQueue *q1_;   // First  FIFO queue
	PacketQueue *q2_;   // Second FIFO queue
};