//
// Author:    Jae Chung
// File:      dtrr-queue.cc
// Written:   07/19/99 (for ns-2.1b4a)
// Modifed:   10/14/01 (for ns-2.1b8a)
// 

#include "my-queue.h"
#include <string>
#include <sstream>
#include <iostream>

static class MyQueueClass : public TclClass {
public:
    MyQueueClass() : TclClass("Queue/MyQueue") {}
    TclObject* create(int, const char* const*) {
        return (new MyQueue);
    }
} class_myqueue;


//Contructor
MyQueue::MyQueue() : Queue() {
	{ 		
		q1_ = new PacketQueue;
		q2_ = new PacketQueue;
		q3_ = new PacketQueue;
		pq_ = q3_;
	}
}

void MyQueue::enque(Packet* p) {
    Tcl& tcl = Tcl::instance();
	hdr_ip* iph = hdr_ip::access(p);	

	if (iph->is_very_high_prio){
		if (q3_->length() >= qlim_) {
			drop(p);
		} else {
			q3_->enque(p);
		}
	} else if (iph->is_high_prio){
		if (q2_->length() >= qlim_) {
			drop(p);
		} else {
			q2_->enque(p);
		}
	} else {
		if (q1_->length() >= qlim_) {
			drop(p);
		} else {
			q1_->enque(p);
		}
	}
}


Packet* MyQueue::deque()
{
	Packet* p = NULL; 
	Tcl& tcl = Tcl::instance();

	if (q3_->length() > 0) {
		p = q3_->deque();
	} else if (q2_->length() > 0) {
		p = q2_->deque();
	} else if (q1_->length() > 0) {
		p = q1_->deque();
	}

	return p; 
}

