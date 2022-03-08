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
		pq_ = q1_;
	}
}

void MyQueue::enque(Packet* p) {
    Tcl& tcl = Tcl::instance();
	hdr_ip* iph = hdr_ip::access(p);
	
	if ((q1_->length() + q2_->length()) > qlim_ - 2) {

		std::stringstream command; 
		command << "puts \"Queue Full Dropping\"\n " << std::endl;
		tcl.eval(command.str().c_str());
		drop(p);
	}

	if (iph->prio_ == 15){
		q1_->enque(p);
		std::stringstream command; 
		command << "puts \"Enqueuing to Q1. New Queue length: " << q1_->length() << "\"\n " << std::endl;
		tcl.eval(command.str().c_str());
	} else {
		q2_->enque(p);
		std::stringstream command; 
		command << "puts \"Enqueuing to Q2. New Queue length: " << q2_->length() << "\"\n " << std::endl;
		tcl.eval(command.str().c_str());
	}
}


Packet* MyQueue::deque()
{
	Packet* p = NULL; 
	Tcl& tcl = Tcl::instance();

	if (q1_->length() > 0) {
		p = q1_->deque();
		
		std::stringstream command; 
		command << "puts \"Dequeuing from Q1. New Queue length: " << q1_->length() << "\"\n " << std::endl;
		tcl.eval(command.str().c_str());

	}
	else if (q2_->length() > 0) {
		p = q2_->deque();

		std::stringstream command; 
		command << "puts \"Dequeuing from Q2. New Queue length: " << q2_->length() << "\"\n " << std::endl;
		tcl.eval(command.str().c_str());
		
	}  

	return p; 
}





