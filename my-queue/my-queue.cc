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
#include "delay.h"

static class MyQueueClass : public TclClass {
public:
    MyQueueClass() : TclClass("Queue/MyQueue") {}
    TclObject* create(int, const char* const*) {
        return (new MyQueue);
    }
} class_myqueue;


//Contructor
MyQueue::MyQueue() : Queue() {
	q1_ = new PacketQueue;
	q2_ = new PacketQueue;
	q3_ = new PacketQueue;
	pq_ = q3_;
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


Packet* MyQueue::deque(){
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



///////////////////////////////////////////////////////////
// MamadQueue Implementation //////////////////////////////
///////////////////////////////////////////////////////////

static class MamadQueueClass : public TclClass {
public:
    MamadQueueClass() : TclClass("Queue/MamadQueue") {}
    TclObject* create(int, const char* const*) {
        return (new MamadQueue);
    }
} class_mamadqueue;


//Contructor
MamadQueue::MamadQueue() : Queue() {
	// std::cout << "v4 constructor" << std::endl;
	pq_ = vhpq.q_;
	bindparams();
}

void MamadQueue::enque(Packet* p) {
	// std::cout << "v4 enque" << std::endl;
    Tcl& tcl = Tcl::instance();
	hdr_ip* iph = hdr_ip::access(p);	

	if (iph->is_very_high_prio){
		// std::cout << "v4 enque vhp" << std::endl;
		vhpq.enque(p);
	} else if (iph->is_high_prio){
		// std::cout << "v4 enque hp" << std::endl;
		hpq.enque(p);
	} else {
		// std::cout << "v4 enque lp" << std::endl;
		lpq.enque(p); 
	}
}


Packet* MamadQueue::deque(){
	Packet* p = NULL; 
	Tcl& tcl = Tcl::instance();

	if (vhpq.length() > 0) {
		// std::cout << "v4 deque vhp" << std::endl;
		p = vhpq.deque();
	} else if (hpq.length() > 0) {
		// std::cout << "v4 deque hp" << std::endl;
		p = hpq.deque();
	} else if (lpq.length() > 0) {
		// std::cout << "v4 deque lp" << std::endl;
		p = lpq.deque();
	}

	return p; 
}



int MamadQueue::command(int argc, const char*const* argv) {
	int ret=TCL_OK;

	// std::cout << "MamadQueue command" << std::endl;
	// // print all arguments 
	// for (int i = 0; i < argc; i++) {
	// 	std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
	// }
	// std:cout << "---------------------------" << std::endl;

	if (strcmp(argv[1], "per-queue") == 0)	{
		printf("per-queue command is empty!\n");
	} else {
		Tcl& tcl = Tcl::instance();
		if (argc == 2) {
			if (strcmp(argv[1], "reset") == 0) {
				reset();
				return (TCL_OK);
			}
			if (strcmp(argv[1], "early-drop-target") == 0) {
				if (lpq.de_drop_ != NULL)
					tcl.resultf("%s", lpq.de_drop_->name());
				return (TCL_OK);
			}
			if (strcmp(argv[1], "edrop-trace") == 0) {
				if (lpq.EDTrace != NULL) {
					tcl.resultf("%s", lpq.EDTrace->name());
					if (debug_)
						printf("edrop trace exists according to RED\n");
				}
				else {
					if (debug_)
						printf("edrop trace doesn't exist according to RED\n");
					tcl.resultf("0");
				}
				return (TCL_OK);
			}
			if (strcmp(argv[1], "trace-type") == 0) {
				tcl.resultf("%s", lpq.traceType);
				return (TCL_OK);
			}
			if (strcmp(argv[1], "printstats") == 0) {
				lpq.print_summarystats();
				return (TCL_OK);
			}
		}
		else if (argc == 3) {
			if (strcmp(argv[1], "nodeEntry") == 0) {
				nodeEntry_ = (Classifier*)TclObject::lookup(argv[2]);
				if (nodeEntry_ == 0) {
					tcl.resultf("no such object %s", argv[2]);
					return (TCL_ERROR);
				}
				return (TCL_OK);
			}
			if (strcmp(argv[1], "get-size") == 0) {
				return(TCL_OK);
			}
			if (strcmp(argv[1], "get-threshold") == 0){
				return(TCL_OK);
			}

			// attach a file for variable tracing
			if (strcmp(argv[1], "attach") == 0) {
				int mode;
				const char* id = argv[2];
				lpq.tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
				if (lpq.tchan_ == 0) {
					tcl.resultf("RED: trace: can't attach %s for writing", id);
					return (TCL_ERROR);
				}
				return (TCL_OK);
			}
			// tell RED about link stats
			if (strcmp(argv[1], "link") == 0) {
				std::cout << "v4 link" << std::endl;
		
				LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);

				if (del == 0) {
					tcl.resultf("RED: no LinkDelay object %s",
						argv[2]);
					return(TCL_ERROR);
				}
				// set ptc now
				lpq.link_ = del;
				lpq.edp_.ptc = lpq.link_->bandwidth() /
					(8.0 * lpq.edp_.mean_pktsize);
				lpq.edp_.delay = lpq.link_->delay();
				if (
				  (lpq.edp_.q_w <= 0.0 || lpq.edp_.th_min_pkts == 0 ||
						  lpq.edp_.th_max_pkts == 0))
					initialize_params();

				return (TCL_OK);
			}
			if (strcmp(argv[1], "early-drop-target") == 0) {
				NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
				if (p == 0) {
					tcl.resultf("no object %s", argv[2]);
					return (TCL_ERROR);
				}
				lpq.de_drop_ = p;
				return (TCL_OK);
			}
			if (strcmp(argv[1], "edrop-trace") == 0) {
				if (debug_)
					printf("Ok, Here\n");
				NsObject * t  = (NsObject *)TclObject::lookup(argv[2]);
				if (debug_)
					printf("Ok, Here too\n");
				if (t == 0) {
					tcl.resultf("no object %s", argv[2]);
					return (TCL_ERROR);
				}
				lpq.EDTrace = t;
				if (debug_)
					printf("Ok, Here too too too %d\n", ((Trace *)lpq.EDTrace)->type_);
				return (TCL_OK);
			}
			if (!strcmp(argv[1], "packetqueue-attach")) {
				// delete m_Queues->q_;
				// if (!(m_Queues->q_ = (PacketQueue*) TclObject::lookup(argv[2])))
				// 	return (TCL_ERROR);
				// else {
				// 	pq_ = m_Queues->q_;
				// 	return (TCL_OK);
				// }

				return (TCL_OK);
			}
		}
//		fprintf(pFile,"command End: %d\n",ret);
//		fclose(pFile);
		return (Queue::command(argc, argv));
	}
	return ret;
}

void MamadQueue::bindparams(){	
	// std::cout << "MamadQueue::bindparams()" << std::endl;

	// lpq params 
	bind_bool("bytes_", &lpq.edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &lpq.qib_);	    // boolean: q in bytes?
	bind("thresh_", &lpq.edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_", &lpq.edp_.th_min);
	bind("maxthresh_", &lpq.edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_", &lpq.edp_.th_max);
	bind("mean_pktsize_", &lpq.edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_", &lpq.edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_", &lpq.edp_.q_w);		    // for EWMA
	bind("adaptive_", &lpq.edp_.adaptive);          // 1 for adaptive red
	bind("cautious_", &lpq.edp_.cautious);          // 1 for cautious marking
	bind("alpha_", &lpq.edp_.alpha); 	  	    // adaptive red param
	bind("beta_", &lpq.edp_.beta);                  // adaptive red param
	bind("interval_", &lpq.edp_.interval);	    // adaptive red param
	bind("feng_adaptive_",&lpq.edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_", &lpq.edp_.targetdelay);    // target delay
	bind("top_", &lpq.edp_.top);		    // maximum for max_p
	bind("bottom_", &lpq.edp_.bottom);		    // minimum for max_p
	bind_bool("wait_", &lpq.edp_.wait);
	bind("linterm_", &lpq.edp_.max_p_inv);
	bind("mark_p_", &lpq.edp_.mark_p);
	bind_bool("use_mark_p_", &lpq.edp_.use_mark_p);
	bind_bool("setbit_", &lpq.edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_", &lpq.edp_.gentle);         // increase the packet
	bind_bool("drop_tail_", &lpq.drop_tail_);	    // drop last pkt
	bind_bool("drop_front_", &lpq.drop_front_);	    // drop first pkt
	bind_bool("drop_rand_", &lpq.drop_rand_);	    // drop pkt at random
	bind_bool("ns1_compat_", &lpq.ns1_compat_);	    // ns-1 compatibility
	bind("ave_", &lpq.edv_.v_ave);		    // average queue sie
	bind("prob1_", &lpq.edv_.v_prob1);		    // dropping probability
	bind("curq_", &lpq.curq_);			    // current queue size
	bind("cur_max_p_", &lpq.edv_.cur_max_p);        // current max_p




	// vhpq params 
	bind_bool("bytes_1_", &vhpq.edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_1_", &vhpq.qib_);	    // boolean: q in bytes?
	bind("thresh_1_", &vhpq.edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_1_", &vhpq.edp_.th_min);
	bind("maxthresh_1_", &vhpq.edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_1_", &vhpq.edp_.th_max);
	bind("mean_pktsize_1_", &vhpq.edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_1_", &vhpq.edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_1_", &vhpq.edp_.q_w);		    // for EWMA
	bind("adaptive_1_", &vhpq.edp_.adaptive);          // 1 for adaptive red
	bind("cautious_1_", &vhpq.edp_.cautious);          // 1 for cautious marking
	bind("alpha_1_", &vhpq.edp_.alpha); 	  	    // adaptive red param
	bind("beta_1_", &vhpq.edp_.beta);                  // adaptive red param
	bind("interval_1_", &vhpq.edp_.interval);	    // adaptive red param
	bind("feng_adaptive_1_", &vhpq.edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_1_", &vhpq.edp_.targetdelay);    // target delay
	bind("top_1_", &vhpq.edp_.top);		    // maximum for max_p
	bind("bottom_1_", &vhpq.edp_.bottom);		    // minimum for max_p
	bind_bool("wait_1_", &vhpq.edp_.wait);
	bind("linterm_1_", &vhpq.edp_.max_p_inv);
	bind("mark_p_1_", &vhpq.edp_.mark_p);
	bind_bool("use_mark_p_1_", &vhpq.edp_.use_mark_p);
	bind_bool("setbit_1_", &vhpq.edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_1_", &vhpq.edp_.gentle);         // increase the packet
	bind_bool("drop_tail_1_", &vhpq.drop_tail_);	    // drop last pkt
	bind_bool("drop_front_1_", &vhpq.drop_front_);	    // drop first pkt
	bind_bool("drop_rand_1_", &vhpq.drop_rand_);	    // drop pkt at random
	bind_bool("ns1_compat_1_", &vhpq.ns1_compat_);	    // ns-1 compatibility
	bind("ave_1_", &vhpq.edv_.v_ave);		    // average queue sie
	bind("prob1_1_", &vhpq.edv_.v_prob1);		    // dropping probability
	bind("curq_1_", &vhpq.curq_);			    // current queue size
	bind("cur_max_p_1_", &vhpq.edv_.cur_max_p);        // current max_p




	// hpq params 
	bind_bool("bytes_2_", &hpq.edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_2_", &hpq.qib_);	    // boolean: q in bytes?
	bind("thresh_2_", &hpq.edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_2_", &hpq.edp_.th_min);
	bind("maxthresh_2_", &hpq.edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_2_", &hpq.edp_.th_max);
	bind("mean_pktsize_2_", &hpq.edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_2_", &hpq.edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_2_", &hpq.edp_.q_w);		    // for EWMA
	bind("adaptive_2_", &hpq.edp_.adaptive);          // 1 for adaptive red
	bind("cautious_2_", &hpq.edp_.cautious);          // 1 for cautious marking
	bind("alpha_2_", &hpq.edp_.alpha); 	  	    // adaptive red param
	bind("beta_2_", &hpq.edp_.beta);                  // adaptive red param
	bind("interval_2_", &hpq.edp_.interval);	    // adaptive red param
	bind("feng_adaptive_2_", &hpq.edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_2_", &hpq.edp_.targetdelay);    // target delay
	bind("top_2_", &hpq.edp_.top);		    // maximum for max_p
	bind("bottom_2_", &hpq.edp_.bottom);		    // minimum for max_p
	bind_bool("wait_2_", &hpq.edp_.wait);
	bind("linterm_2_", &hpq.edp_.max_p_inv);
	bind("mark_p_2_", &hpq.edp_.mark_p);
	bind_bool("use_mark_p_2_", &hpq.edp_.use_mark_p);
	bind_bool("setbit_2_", &hpq.edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_2_", &hpq.edp_.gentle);         // increase the packet
	bind_bool("drop_tail_2_", &hpq.drop_tail_);	    // drop last pkt
	bind_bool("drop_front_2_", &hpq.drop_front_);	    // drop first pkt
	bind_bool("drop_rand_2_", &hpq.drop_rand_);	    // drop pkt at random
	bind_bool("ns1_compat_2_", &hpq.ns1_compat_);	    // ns-1 compatibility
	bind("ave_2_", &hpq.edv_.v_ave);		    // average queue sie
	bind("prob1_2_", &hpq.edv_.v_prob1);		    // dropping probability
	bind("curq_2_", &hpq.curq_);			    // current queue size
	bind("cur_max_p_2_", &hpq.edv_.cur_max_p);        // current max_p

}

void MamadQueue::reset() {
	vhpq.reset();
	hpq.reset();
	lpq.reset();
}

void MamadQueue::print_summarystats() {	
	vhpq.print_summarystats();
	hpq.print_summarystats();
	lpq.print_summarystats();
}

void MamadQueue::initialize_params() {	
	// std::cout << "MamadQueue::initialize_params()" << std::endl;
	vhpq.initialize_params();
	hpq.initialize_params();
	lpq.initialize_params();
}


///////////////////////////////////////////////////////////
// MyREDQueue Implementation //////////////////////////////
///////////////////////////////////////////////////////////


static class MyREDQueueClass : public TclClass {
public:
    MyREDQueueClass() : TclClass("Queue/MyREDQueue") {}
    TclObject* create(int, const char* const*) {
        return (new MyREDQueue);
    }
} class_myredqueue;


//Contructor
MyREDQueue::MyREDQueue() : Queue() {
	q1_ = new PacketQueue;
	q2_ = new PacketQueue;
	q3_ = new PacketQueue;
	pq_ = q3_;
}

void MyREDQueue::enque(Packet* p) {
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


Packet* MyREDQueue::deque(){
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


///////////////////////////////////////////////////////////
// MyOtherREDQueue Implementation /////////////////////////
///////////////////////////////////////////////////////////


static class MyOtherREDQueueClass : public TclClass {
public:
    MyOtherREDQueueClass() : TclClass("Queue/MyOtherREDQueue") {}
    TclObject* create(int, const char* const*) {
        return (new MyOtherREDQueue);
    }
} class_myotherredqueue;


MyOtherREDQueue::MyOtherREDQueue() : REDQueue() {
	q1_ = new PacketQueue;
	q2_ = new PacketQueue;
	q3_ = new PacketQueue;
}



void MyOtherREDQueue::enque(Packet* p) {
	// std::cout << "MyOtherREDQueue::enque" << std::endl;
	REDQueue::enque(p);
}


Packet* MyOtherREDQueue::deque() {
	return REDQueue::deque();
}







