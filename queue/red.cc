 /* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Here is one set of parameters from one of Sally's simulations
 * (this is from tcpsim, the older simulator):
 * 
 * ed [ q_weight=0.002 thresh=5 linterm=30 maxthresh=15
 *         mean_pktsize=500 dropmech=random-drop queue-size=60
 *         plot-file=none bytes=false doubleq=false dqthresh=50 
 *	   wait=true ]
 * 
 * 1/"linterm" is the max probability of dropping a packet. 
 * There are different options that make the code
 * more messy that it would otherwise be.  For example,
 * "doubleq" and "dqthresh" are for a queue that gives priority to
 *   small (control) packets, 
 * "bytes" indicates whether the queue should be measured in bytes 
 *   or in packets, 
 * "dropmech" indicates whether the drop function should be random-drop 
 *   or drop-tail when/if the queue overflows, and 
 *   the commented-out Holt-Winters method for computing the average queue 
 *   size can be ignored.
 * "wait" indicates whether the gateway should wait between dropping
 *   packets.
 */

#ifndef lint
static const char rcsid[] =
     "@(#) $Header: /cvsroot/nsnam/ns-2/queue/red.cc,v 1.88 2007/10/23 06:55:54 seashadow Exp $ (LBL)";
#endif

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "red.h"

static class REDClass : public TclClass {
public:
	REDClass() : TclClass("Queue/RED") {}
	TclObject* create(int argc, const char*const* argv) {
		//printf("creating RED Queue. argc = %d\n", argc);
		
		//mod to enable RED to take arguments
		if (argc==5) 
			return (new REDQueue(argv[4]));
		else
			return (new REDQueue("Drop"));
	}
} class_red;

/* Strangely this didn't work. 
 * Seg faulted for child classes.
REDQueue::REDQueue() { 
	REDQueue("Drop");
}
*/

/*
 * modified to enable instantiation with special Trace objects - ratul
 */
REDQueue::REDQueue(const char * trace) : link_(NULL), de_drop_(NULL), EDTrace(NULL), tchan_(0), idle_(1), idletime_(0.0)
{
	initParams();
	
	//	printf("Making trace type %s\n", trace);
	if (strlen(trace) >=20) {
		printf("trace type too long - allocate more space to traceType in red.h and recompile\n");
		exit(0);
	}
	strcpy(traceType, trace);
	bind_bool("bytes_", &edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &qib_);	    // boolean: q in bytes?
	//	_RENAMED("queue-in-bytes_", "queue_in_bytes_");

	bind("thresh_", &edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_", &edp_.th_min);
	bind("maxthresh_", &edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_", &edp_.th_max);
	bind("mean_pktsize_", &edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_", &edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_", &edp_.q_w);		    // for EWMA
	bind("adaptive_", &edp_.adaptive);          // 1 for adaptive red
	bind("cautious_", &edp_.cautious);          // 1 for cautious marking
	bind("alpha_", &edp_.alpha); 	  	    // adaptive red param
	bind("beta_", &edp_.beta);                  // adaptive red param
	bind("interval_", &edp_.interval);	    // adaptive red param
	bind("feng_adaptive_",&edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_", &edp_.targetdelay);    // target delay
	bind("top_", &edp_.top);		    // maximum for max_p	
	bind("bottom_", &edp_.bottom);		    // minimum for max_p	
	bind_bool("wait_", &edp_.wait);
	bind("linterm_", &edp_.max_p_inv);
	bind("mark_p_", &edp_.mark_p);
	bind_bool("use_mark_p_", &edp_.use_mark_p);
	bind_bool("setbit_", &edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_", &edp_.gentle);         // increase the packet
						    // drop prob. slowly
						    // when ave queue
						    // exceeds maxthresh

	bind_bool("summarystats_", &summarystats_);
	bind_bool("drop_tail_", &drop_tail_);	    // drop last pkt
	//	_RENAMED("drop-tail_", "drop_tail_");

	bind_bool("drop_front_", &drop_front_);	    // drop first pkt
	//	_RENAMED("drop-front_", "drop_front_");
	
	bind_bool("drop_rand_", &drop_rand_);	    // drop pkt at random
	//	_RENAMED("drop-rand_", "drop_rand_");

	bind_bool("ns1_compat_", &ns1_compat_);	    // ns-1 compatibility
	//	_RENAMED("ns1-compat_", "ns1_compat_");

	bind("ave_", &edv_.v_ave);		    // average queue sie
	bind("prob1_", &edv_.v_prob1);		    // dropping probability
	bind("curq_", &curq_);			    // current queue size
	bind("cur_max_p_", &edv_.cur_max_p);        // current max_p
	

	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	//reset();
#ifdef notdef
	print_edp();
	print_edv();
#endif
	
}


/*
 * Note: if the link bandwidth changes in the course of the
 * simulation, the bandwidth-dependent RED parameters do not change.
 * This should be fixed, but it would require some extra parameters,
 * and didn't seem worth the trouble...
 */
void REDQueue::initialize_params()
{
/*
 * If q_weight=0, set it to a reasonable value of 1-exp(-1/C)
 * This corresponds to choosing q_weight to be of that value for
 * which the packet time constant -1/ln(1-q_weight) per default RTT 
 * of 100ms is an order of magnitude more than the link capacity, C.
 *
 * If q_weight=-1, then the queue weight is set to be a function of
 * the bandwidth and the link propagation delay.  In particular, 
 * the default RTT is assumed to be three times the link delay and 
 * transmission delay, if this gives a default RTT greater than 100 ms. 
 *
 * If q_weight=-2, set it to a reasonable value of 1-exp(-10/C).
 */
	if (edp_.q_w == 0.0) {
		edp_.q_w = 1.0 - exp(-1.0/edp_.ptc);
 	} else if (edp_.q_w == -1.0) {
		double rtt = 3.0*(edp_.delay+1.0/edp_.ptc);
		//printf("delay: %5.4f rtt: %5.4f\n", edp_.delay, rtt);
		if (rtt < 0.1) 
			rtt = 0.1;
		edp_.q_w = 1.0 - exp(-1.0/(10*rtt*edp_.ptc));
	} else if (edp_.q_w == -2.0) {
		edp_.q_w = 1.0 - exp(-10.0/edp_.ptc);
	}

	// printf("ptc: %7.5f bandwidth: %5.3f pktsize: %d\n", edp_.ptc, link_->bandwidth(), edp_.mean_pktsize);
        // printf("th_min_pkts: %7.5f th_max_pkts: %7.5f\n", edp_.th_min_pkts, edp_.th_max);
	if (edp_.th_min_pkts == 0) {
		edp_.th_min_pkts = 5.0;
		// set th_min_pkts to half of targetqueue, if this is greater
		//  than 5 packets.
		double targetqueue = edp_.targetdelay * edp_.ptc;
		if (edp_.th_min_pkts < targetqueue / 2.0 )
			edp_.th_min_pkts = targetqueue / 2.0 ;
        }
	if (edp_.th_max_pkts == 0) 
		edp_.th_max_pkts = 3.0 * edp_.th_min_pkts;
        //printf("th_min_pkts: %7.5f th_max_pkts: %7.5f\n", edp_.th_min_pkts, edp_.th_max);
	//printf("q_w: %7.5f\n", edp_.q_w);
	if (edp_.bottom == 0) {
		edp_.bottom = 0.01;
		// Set bottom to at most 1/W, for W the delay-bandwidth 
		//   product in packets for a connection with this bandwidth,
		//   1000-byte packets, and 100 ms RTTs.
		// So W = 0.1 * link_->bandwidth() / 8000 
		double bottom1 = 80000.0/link_->bandwidth();
		if (bottom1 < edp_.bottom) 
			edp_.bottom = bottom1;
		//printf("bottom: %9.7f\n", edp_.bottom);
	}
}

void REDQueue::initParams() 
{
	edp_.mean_pktsize = 0;
	edp_.idle_pktsize = 0;
	edp_.bytes = 0;
	edp_.wait = 0;
	edp_.setbit = 0;
	edp_.gentle = 0;
	edp_.th_min = 0.0;
	edp_.th_min_pkts = 0.0;
	edp_.th_max = 0.0;
	edp_.th_max_pkts = 0.0;
	edp_.max_p_inv = 0.0;
	edp_.q_w = 0.0;
	edp_.adaptive = 0;
	edp_.cautious = 0;
	edp_.alpha = 0.0;
	edp_.beta = 0.0;
	edp_.interval = 0.0;
	edp_.targetdelay = 0.0;
	edp_.top = 0.0;
	edp_.bottom = 0.0;
	edp_.feng_adaptive = 0;
	edp_.ptc = 0.0;
	edp_.delay = 0.0;
	
	edv_.v_ave = 0.0;
	edv_.v_prob1 = 0.0;
	edv_.v_slope = 0.0;
	edv_.v_prob = 0.0;
	edv_.v_a = 0.0;
	edv_.v_b = 0.0;
	edv_.v_c = 0.0;
	edv_.v_d = 0.0;
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.old = 0;
	edv_.cur_max_p = 1.0;
	edv_.lastset = 0;
}


void REDQueue::reset()
{
	
        //printf("3: th_min_pkts: %5.2f\n", edp_.th_min_pkts); 
	/*
	 * Compute the "packet time constant" if we know the
	 * link bandwidth.  The ptc is the max number of (avg sized)
	 * pkts per second which can be placed on the link.
	 * The link bw is given in bits/sec, so scale mean psize
	 * accordingly.
	 */
        if (link_) {
		edp_.ptc = link_->bandwidth() / (8.0 * edp_.mean_pktsize);
		initialize_params();
	}
	if (edp_.th_max_pkts == 0) 
		edp_.th_max_pkts = 3.0 * edp_.th_min_pkts;
	/*
	 * If queue is measured in bytes, scale min/max thresh
	 * by the size of an average packet (which is specified by user).
	 */
        if (qib_) {
		//printf("1: th_min in pkts: %5.2f mean_pktsize: %d \n", edp_.th_min_pkts, edp_.mean_pktsize); 
                edp_.th_min = edp_.th_min_pkts * edp_.mean_pktsize;  
                edp_.th_max = edp_.th_max_pkts * edp_.mean_pktsize;
		//printf("2: th_min in bytes (if qib): %5.2f mean_pktsize: %d \n", edp_.th_min, edp_.mean_pktsize); 
        } else {
		edp_.th_min = edp_.th_min_pkts;
		edp_.th_max = edp_.th_max_pkts;
	}
	 
	edv_.v_ave = 0.0;
	edv_.v_slope = 0.0;
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.old = 0;
	double th_diff = (edp_.th_max - edp_.th_min);
	if (th_diff == 0) { 
		//XXX this last check was added by a person who knows
		//nothing of this code just to stop FP div by zero.
		//Values for thresholds were equal at time 0.  If you
		//know what should be here, please cleanup and remove
		//this comment.
		th_diff = 1.0; 
	}
	edv_.v_a = 1.0 / th_diff;
	edv_.cur_max_p = 1.0 / edp_.max_p_inv;
	edv_.v_b = - edp_.th_min / th_diff;
	edv_.lastset = 0.0;
	if (edp_.gentle) {
		edv_.v_c = ( 1.0 - edv_.cur_max_p ) / edp_.th_max;
		edv_.v_d = 2.0 * edv_.cur_max_p - 1.0;
	}

	idle_ = 1;
	if (&Scheduler::instance() != NULL)
		idletime_ = Scheduler::instance().clock();
	else
		idletime_ = 0.0; /* sched not instantiated yet */
	
	if (debug_) 
		printf("Doing a queue reset\n");
	Queue::reset();
	if (debug_) 
		printf("Done queue reset\n");
}

/*
 *  Updating max_p, following code from Feng et al. 
 *  This is only called for Adaptive RED.
 *  From "A Self-Configuring RED Gateway", from Feng et al.
 *  They recommend alpha = 3, and beta = 2.
 */
void REDQueue::updateMaxPFeng(double new_ave)
{
	if ( edp_.th_min < new_ave && new_ave < edp_.th_max) {
		edv_.status = edv_.Between;
	}
	if (new_ave < edp_.th_min && edv_.status != edv_.Below) {
		edv_.status = edv_.Below;
		edv_.cur_max_p = edv_.cur_max_p / edp_.alpha;
		//double max = edv_.cur_max_p; double param = edp_.alpha;
		//printf("max: %5.2f alpha: %5.2f\n", max, param);
	}
	if (new_ave > edp_.th_max && edv_.status != edv_.Above) {
		edv_.status = edv_.Above;
		edv_.cur_max_p = edv_.cur_max_p * edp_.beta;
		//double max = edv_.cur_max_p; double param = edp_.alpha;
		//printf("max: %5.2f beta: %5.2f\n", max, param);
	}
}

/*
 *  Updating max_p to keep the average queue size within the target range.
 *  This is only called for Adaptive RED.
 */
void REDQueue::updateMaxP(double new_ave, double now)
{
	double part = 0.4*(edp_.th_max - edp_.th_min);
	// AIMD rule to keep target Q~1/2(th_min+th_max)
	if ( new_ave < edp_.th_min + part && edv_.cur_max_p > edp_.bottom) {
		// we increase the average queue size, so decrease max_p
		edv_.cur_max_p = edv_.cur_max_p * edp_.beta;
		edv_.lastset = now;
	} else if (new_ave > edp_.th_max - part && edp_.top > edv_.cur_max_p ) {
		// we decrease the average queue size, so increase max_p
		double alpha = edp_.alpha;
                        if ( alpha > 0.25*edv_.cur_max_p )
			alpha = 0.25*edv_.cur_max_p;
		edv_.cur_max_p = edv_.cur_max_p + alpha;
		edv_.lastset = now;
	} 
}

/*
 * Compute the average queue size.
 * Nqueued can be bytes or packets.
 */
double REDQueue::estimator(int nqueued, int m, double ave, double q_w)
{
	double new_ave, old_ave;

	new_ave = ave;
	while (--m >= 1) {
		new_ave *= 1.0 - q_w;
	}
	old_ave = new_ave;
	new_ave *= 1.0 - q_w;
	new_ave += q_w * nqueued;
	
	double now = Scheduler::instance().clock();
	if (edp_.adaptive == 1) {
		if (edp_.feng_adaptive == 1)
			updateMaxPFeng(new_ave);
		else if (now > edv_.lastset + edp_.interval)
			updateMaxP(new_ave, now);
	}
	return new_ave;
}

/*
 * Return the next packet in the queue for transmission.
 */
Packet* REDQueue::deque()
{
	Packet *p;
	if (summarystats_ && &Scheduler::instance() != NULL) {
		Queue::updateStats(qib_?q_->byteLength():q_->length());
	}
	p = q_->deque();
	if (p != 0) {
		idle_ = 0;
	} else {
		idle_ = 1;
		// deque() may invoked by Queue::reset at init
		// time (before the scheduler is instantiated).
		// deal with this case
		if (&Scheduler::instance() != NULL)
			idletime_ = Scheduler::instance().clock();
		else
			idletime_ = 0.0;
	}
	return (p);
}

/*
 * Calculate the drop probability.
 */
double
REDQueue::calculate_p_new(double v_ave, double th_max, int gentle, double v_a, 
	double v_b, double v_c, double v_d, double max_p)
{
	double p;
	if (gentle && v_ave >= th_max) {
		// p ranges from max_p to 1 as the average queue
		// size ranges from th_max to twice th_max 
		p = v_c * v_ave + v_d;
        } else if (!gentle && v_ave >= th_max) { 
                // OLD: p continues to range linearly above max_p as
                // the average queue size ranges above th_max.
                // NEW: p is set to 1.0 
                p = 1.0;
        } else {
                // p ranges from 0 to max_p as the average queue
                // size ranges from th_min to th_max 
                p = v_a * v_ave + v_b;
                // p = (v_ave - th_min) / (th_max - th_min)
                p *= max_p; 
        }
	if (p > 1.0)
		p = 1.0;
	return p;
}

/*
 * Calculate the drop probability.
 * This is being kept for backwards compatibility.
 */
double
REDQueue::calculate_p(double v_ave, double th_max, int gentle, double v_a, 
	double v_b, double v_c, double v_d, double max_p_inv)
{
	double p = calculate_p_new(v_ave, th_max, gentle, v_a,
		v_b, v_c, v_d, 1.0 / max_p_inv);
	return p;
}

/*
 * Make uniform instead of geometric interdrop periods.
 */
double
REDQueue::modify_p(double p, int count, int count_bytes, int bytes, 
   int mean_pktsize, int wait, int size)
{
	double count1 = (double) count;
	if (bytes)
		count1 = (double) (count_bytes/mean_pktsize);
	if (wait) {
		if (count1 * p < 1.0)
			p = 0.0;
		else if (count1 * p < 2.0)
			p /= (2.0 - count1 * p);
		else
			p = 1.0;
	} else {
		if (count1 * p < 1.0)
			p /= (1.0 - count1 * p);
		else
			p = 1.0;
	}
	if (bytes && p < 1.0) {
		p = (p * size) / mean_pktsize;
		//p = p * (size / mean_pktsize);

	}
	if (p > 1.0)
		p = 1.0;
 	return p;
}

/*
 * 
 */

/*
 * should the packet be dropped/marked due to a probabilistic drop?
 */
int
REDQueue::drop_early(Packet* pkt)
{
	hdr_cmn* ch = hdr_cmn::access(pkt);

	edv_.v_prob1 = calculate_p_new(edv_.v_ave, edp_.th_max, edp_.gentle, 
  	  edv_.v_a, edv_.v_b, edv_.v_c, edv_.v_d, edv_.cur_max_p);
	edv_.v_prob = modify_p(edv_.v_prob1, edv_.count, edv_.count_bytes,
	  edp_.bytes, edp_.mean_pktsize, edp_.wait, ch->size());

	// drop probability is computed, pick random number and act
	if (edp_.cautious == 1) {
		 // Don't drop/mark if the instantaneous queue is much
		 //  below the average.
		 // For experimental purposes only.
		int qsize = qib_?q_->byteLength():q_->length();
		// pkts: the number of packets arriving in 50 ms
		double pkts = edp_.ptc * 0.05;
		double fraction = pow( (1-edp_.q_w), pkts);
		// double fraction = 0.9;
		if ((double) qsize < fraction * edv_.v_ave) {
			// queue could have been empty for 0.05 seconds
			// printf("fraction: %5.2f\n", fraction);
			return (0);
		}
	}
	double u = Random::uniform();
	if (edp_.cautious == 2) {
                // Decrease the drop probability if the instantaneous
		//   queue is much below the average.
		// For experimental purposes only.
		int qsize = qib_?q_->byteLength():q_->length();
		// pkts: the number of packets arriving in 50 ms
		double pkts = edp_.ptc * 0.05;
		double fraction = pow( (1-edp_.q_w), pkts);
		// double fraction = 0.9;
		double ratio = qsize / (fraction * edv_.v_ave);
		if (ratio < 1.0) {
			// printf("ratio: %5.2f\n", ratio);
			u *= 1.0 / ratio;
		}
	}
	if (u <= edv_.v_prob) {
		// DROP or MARK
		edv_.count = 0;
		edv_.count_bytes = 0;
		hdr_flags* hf = hdr_flags::access(pickPacketForECN(pkt));
		if (edp_.setbit && hf->ect() && 
		    (!edp_.use_mark_p || edv_.v_prob1 <= edp_.mark_p)) { // Mohammad: I changed < to <= 
			hf->ce() = 1; 	// mark Congestion Experienced bit
			// Tell the queue monitor here - call emark(pkt)
			return (0);	// no drop
		} else {
			return (1);	// drop
		}
	}
	return (0);			// no DROP/mark
}

/*
 * Pick packet for early congestion notification (ECN). This packet is then
 * marked or dropped. Having a separate function do this is convenient for
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the packet for 
 * ECN notification.
 */
Packet*
REDQueue::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

/*
 * Pick packet to drop. Having a separate function do this is convenient for
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the victim.
 */
Packet*
REDQueue::pickPacketToDrop() 
{
	int victim;

	if (drop_front_)
		victim = min(1, q_->length()-1);
	else if (drop_rand_)
		victim = Random::integer(q_->length());
	else			/* default is drop_tail_ */
		victim = q_->length() - 1;

	return(q_->lookup(victim)); 
}

/*
 * Receive a new packet arriving at the queue.
 * The average queue size is computed.  If the average size
 * exceeds the threshold, then the dropping probability is computed,
 * and the newly-arriving packet is dropped with that probability.
 * The packet is also dropped if the maximum queue size is exceeded.
 *
 * "Forced" drops mean a packet arrived when the underlying queue was
 * full, or when the average queue size exceeded some threshold and no
 * randomization was used in selecting the packet to be dropped.
 * "Unforced" means a RED random drop.
 *
 * For forced drops, either the arriving packet is dropped or one in the
 * queue is dropped, depending on the setting of drop_tail_.
 * For unforced drops, the arriving packet is always the victim.
 */

#define	DTYPE_NONE	0	/* ok, no drop */
#define	DTYPE_FORCED	1	/* a "forced" drop */
#define	DTYPE_UNFORCED	2	/* an "unforced" (random) drop */

void REDQueue::enque(Packet* pkt)
{

	/*
	 * if we were idle, we pretend that m packets arrived during
	 * the idle period.  m is set to be the ptc times the amount
	 * of time we've been idle for
	 */

	/*  print_edp(); */
	int m = 0;
	if (idle_) {
		// A packet that arrives to an idle queue will never
		//  be dropped.
		double now = Scheduler::instance().clock();
		/* To account for the period when the queue was empty. */
		idle_ = 0;
		// Use idle_pktsize instead of mean_pktsize, for
		//  a faster response to idle times.
		if (edp_.cautious == 3) {
			double ptc = edp_.ptc * 
			   edp_.mean_pktsize / edp_.idle_pktsize;
			m = int(ptc * (now - idletime_));
		} else
                	m = int(edp_.ptc * (now - idletime_));
	}

	/*
	 * Run the estimator with either 1 new packet arrival, or with
	 * the scaled version above [scaled by m due to idle time]
	 */
	edv_.v_ave = estimator(qib_ ? q_->byteLength() : q_->length(), m + 1, edv_.v_ave, edp_.q_w);
	//printf("v_ave: %6.4f (%13.12f) q: %d)\n", 
	//	double(edv_.v_ave), double(edv_.v_ave), q_->length());
	if (summarystats_) {
		/* compute true average queue size for summary stats */
		Queue::updateStats(qib_?q_->byteLength():q_->length());
	}

	/*
	 * count and count_bytes keeps a tally of arriving traffic
	 * that has not been dropped (i.e. how long, in terms of traffic,
	 * it has been since the last early drop)
	 */

	hdr_cmn* ch = hdr_cmn::access(pkt);
	++edv_.count;
	edv_.count_bytes += ch->size();

	/*
	 * DROP LOGIC:
	 *	q = current q size, ~q = averaged q size
	 *	1> if ~q > maxthresh, this is a FORCED drop
	 *	2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
	 *	3> if (q+1) > hard q limit, this is a FORCED drop
	 */

	register double qavg = edv_.v_ave;
	int droptype = DTYPE_NONE;
	int qlen = qib_ ? q_->byteLength() : q_->length();
	int qlim = qib_ ? (qlim_ * edp_.mean_pktsize) : qlim_;

	curq_ = qlen;	// helps to trace queue during arrival, if enabled

	if (qavg >= edp_.th_min && qlen > 1) {
		if (!edp_.use_mark_p && 
			((!edp_.gentle && qavg >= edp_.th_max) ||
			(edp_.gentle && qavg >= 2 * edp_.th_max))) {
			droptype = DTYPE_FORCED;
		} else if (edv_.old == 0) {
			/* 
			 * The average queue size has just crossed the
			 * threshold from below to above "minthresh", or
			 * from above "minthresh" with an empty queue to
			 * above "minthresh" with a nonempty queue.
			 */
			edv_.count = 1;
			edv_.count_bytes = ch->size();
			edv_.old = 1;
		} else if (drop_early(pkt)) {
			droptype = DTYPE_UNFORCED;
		}
	} else {
		/* No packets are being dropped.  */
		edv_.v_prob = 0.0;
		edv_.old = 0;		
	}
	if (qlen >= qlim) {
		// see if we've exceeded the queue size
		droptype = DTYPE_FORCED;
	}

	if (droptype == DTYPE_UNFORCED) {
		/* pick packet for ECN, which is dropping in this case */
		Packet *pkt_to_drop = pickPacketForECN(pkt);
		/* 
		 * If the packet picked is different that the one that just arrived,
		 * add it to the queue and remove the chosen packet.
		 */
		if (pkt_to_drop != pkt) {
			q_->enque(pkt);
			q_->remove(pkt_to_drop);
			pkt = pkt_to_drop; /* XXX okay because pkt is not needed anymore */
		}

		// deliver to special "edrop" target, if defined
		if (de_drop_ != NULL) {
	
		//trace first if asked 
		// if no snoop object (de_drop_) is defined, 
		// this packet will not be traced as a special case.
			if (EDTrace != NULL) 
				((Trace *)EDTrace)->recvOnly(pkt);

			reportDrop(pkt);
			de_drop_->recv(pkt);
		}
		else {
			reportDrop(pkt);
			drop(pkt);
		}
	} else {
		/* forced drop, or not a drop: first enqueue pkt */
		q_->enque(pkt);

		/* drop a packet if we were told to */
		if (droptype == DTYPE_FORCED) {
			/* drop random victim or last one */
			pkt = pickPacketToDrop();
			q_->remove(pkt);
			reportDrop(pkt);
			drop(pkt);
			if (!ns1_compat_) {
				// bug-fix from Philip Liu, <phill@ece.ubc.ca>
				edv_.count = 0;
				edv_.count_bytes = 0;
			}
		}
	}
	return;
}

int REDQueue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			if (de_drop_ != NULL)
				tcl.resultf("%s", de_drop_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "edrop-trace") == 0) {
			if (EDTrace != NULL) {
				tcl.resultf("%s", EDTrace->name());
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
			tcl.resultf("%s", traceType);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "printstats") == 0) {
			print_summarystats();
			return (TCL_OK);
		}
	} 
	else if (argc == 3) {
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("RED: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		// tell RED about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("RED: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			// set ptc now
			link_ = del;
			edp_.ptc = link_->bandwidth() /
				(8.0 * edp_.mean_pktsize);
			edp_.delay = link_->delay();
			if (
			  (edp_.q_w <= 0.0 || edp_.th_min_pkts == 0 ||
					edp_.th_max_pkts == 0))
                        	initialize_params();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			de_drop_ = p;
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
			EDTrace = t;
			if (debug_)  
				printf("Ok, Here too too too %d\n", ((Trace *)EDTrace)->type_);
			return (TCL_OK);
		}
		if (!strcmp(argv[1], "packetqueue-attach")) {
			delete q_;
			if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
				return (TCL_ERROR);
			else {
				pq_ = q_;
				return (TCL_OK);
			}
		}
	}
	return (Queue::command(argc, argv));
}

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace values of avg queue size, drop probability,
 * and the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void
REDQueue::trace(TracedVar* v)
{
	char wrk[500];
	const char *p;

	if (((p = strstr(v->name(), "ave")) == NULL) &&
	    ((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL) &&
	    ((p = strstr(v->name(), "cur_max_p"))==NULL) ) {
		fprintf(stderr, "RED:unknown trace var %s\n",
			v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
		if (strstr(v->name(), "curq") != NULL) {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t,
				double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}

/* for debugging help */
void REDQueue::print_edp()
{
	printf("mean_pktsz: %d\n", edp_.mean_pktsize); 
	printf("bytes: %d, wait: %d, setbit: %d\n",
		edp_.bytes, edp_.wait, edp_.setbit);
	printf("minth: %f, maxth: %f\n", edp_.th_min, edp_.th_max);
	printf("max_p: %f, qw: %f, ptc: %f\n",
		(double) edv_.cur_max_p, edp_.q_w, edp_.ptc);
	printf("qlim: %d, idletime: %f\n", qlim_, idletime_);
	printf("mark_p: %f, use_mark_p: %d\n", edp_.mark_p, edp_.use_mark_p);
	printf("=========\n");
}

void REDQueue::print_edv()
{
	printf("v_a: %f, v_b: %f\n", edv_.v_a, edv_.v_b);
}

void REDQueue::print_summarystats()
{
	//double now = Scheduler::instance().clock();
	printf("True average queue: %5.3f", true_ave_);
	if (qib_) 
		printf(" (in bytes)");
        printf(" time: %5.3f\n", total_time_);
}

/************************************************************/
/*
 * This procedure is obsolete, and only included for backward compatibility.
 * The new procedure is REDQueue::estimator
 */ 
/*
 * Compute the average queue size.
 * The code contains two alternate methods for this, the plain EWMA
 * and the Holt-Winters method.
 * nqueued can be bytes or packets
 */
void REDQueue::run_estimator(int nqueued, int m)
{
	double f, f_sl, f_old;

	f = edv_.v_ave;
	f_sl = edv_.v_slope;
#define RED_EWMA
#ifdef RED_EWMA
	while (--m >= 1) {
		f_old = f;
		f *= 1.0 - edp_.q_w;
	}
	f_old = f;
	f *= 1.0 - edp_.q_w;
	f += edp_.q_w * nqueued;
#endif
#ifdef RED_HOLT_WINTERS
	while (--m >= 1) {
		f_old = f;
		f += f_sl;
		f *= 1.0 - edp_.q_w;
		f_sl *= 1.0 - 0.5 * edp_.q_w;
		f_sl += 0.5 * edp_.q_w * (f - f_old);
	}
	f_old = f;
	f += f_sl;
	f *= 1.0 - edp_.q_w;
	f += edp_.q_w * nqueued;
	f_sl *= 1.0 - 0.5 * edp_.q_w;
	f_sl += 0.5 * edp_.q_w * (f - f_old);
#endif
	edv_.v_ave = f;
	edv_.v_slope = f_sl;
}

//
void REDQueue::SetIdle()
{
	idle_ = 1;
	// deque() may invoked by Queue::reset at init
	// time (before the scheduler is instantiated).
	// deal with this case
	if (&Scheduler::instance() != NULL)
		idletime_ = Scheduler::instance().clock();
	else
		idletime_ = 0.0;
}


/**
 * : RPQ
 */

void PFCTimer::expire(Event*)
{
	a_->pfctimeout(id);
}

void PFCDeQTimer::expire(Event*)
{
	a_->pfcResume(id);
}

/////////////////////////////////////////////////////////
static class RPQClass : public TclClass {
public:
	RPQClass() : TclClass("Queue/RPQ") {}
	TclObject* create(int argc, const char*const* argv) {
		//printf("creating RED Queue. argc = %d\n", argc);
/*
		//mod to enable RED to take arguments
		if (argc==5)
			return (new RPQ(argv[4]));
		else
*/
		return (new RPQ);
	}
} class_rpq;

/************************************************************/
/*

void RPQ::enque(Packet* p)
{
	hdr_ip* hdr = hdr_ip::access(p);
	FILE* pFile;
	char pName[255];
	sprintf(pName,"/home/student/Desktop/log.txt");
//
	//FIXME: First time we need "w" instead of "a"
	if(Writecounter)
		pFile = fopen (pName,"a");
	else
		pFile = fopen (pName,"w");
	Writecounter++;
//
	fprintf(pFile,"Writecounter=%d enque: hdr->prio()=%d\n",Writecounter,hdr->prio());
	int prio = hdr->prio();
	if(prio>=0 && prio<m_nNumQueues )
	{
		fprintf(pFile,"enque:\n");
		m_Queues[prio]->enque(p);
		fprintf(pFile,"enque:2\n");
	}
	else
	{
		fprintf(pFile,"else:\n");
		m_Queues[m_nNumQueues-1]->enque(p);
		fprintf(pFile,"else:3\n");
	}

	fprintf(pFile,"pFile=%p before closing:\n",pFile);
	fclose (pFile);
}

**********************************************************
Packet* RPQ::deque()
{
	for(int i=0;i<m_nNumQueues;i++)
	{
		if(m_Queues[i]->length()!=0)
		{
			return m_Queues[i]->deque();
		}
	}
	return m_Queues[0]->deque();
}
*/
//**********************************************************
void RPQ::pfctimeout(int id)
{
	if(id>=0 && id<m_nNumQueues)
	{
		if(m_Queues[id].length()<(m_nThreshold[id]-m_nMargin) || m_Queues[id].length()==0)
		{
			DBGMARK(DBGPFC,4,"pfctimeout (id:%d):  Changing sts to from %d to Normal...\n",id,m_pfcState[id]);
			m_pfcState[id]=PFC_NORMAL_STS;
		}
		else
		{
			double pasueTime = 0.000001;//1us
			m_pfcTimer[id].resched(pasueTime);
		}
	}
}

void RPQ::pfcResume(int id)
{
	if(m_bPFC)
	{
		Packet *p;
		int bussy=false;
//		DBGMARK(DBGPFC,2,"@%s : Dequeuing...\n",this->name());
		for(int i=0;i<id;i++)
		{
			if(m_Queues[i].length()!=0)
			{
				bussy=true;
			}
		}
		if(!bussy)
		{
//			Scheduler::instance().schedule(&qh_, &intr_, 0.000001);
			if(m_Queues[id].length()!=0)
			{
//				DBGMARK(0,0," try to send it at now:%f\n",NOW);
				if (!blocked_) {
//					DBGMARK(0,0," \n");
					/*
					 * We're not blocked.
					 */
					p=pfcdeque(id);
					if (p != 0) {
						utilUpdate(last_change_, NOW, blocked_);
						last_change_ = NOW;
						blocked_ = 1;
						target_->recv(p, &qh_);
					}
				}
				else
				{
//					DBGMARK(0,0,"Blocked!\n");
				}
			}
		}
		else
		{
			m_pfcDQTimer[id].resched(0.000001);
		}
	}
}

void RPQ::SendPFCMessage(int Priority, int PauseDuration)
{

	Tcl::instance().evalf("%s sendpfcmessage %d %d", this->name(),Priority,PauseDuration);
}

int RPQ::command(int argc, const char*const* argv)
{
	int ret=TCL_OK;
/*
	FILE* pFile;
	char pName[255];
	sprintf(pName,"/home/student/Desktop/log.txt");
	pFile = fopen (pName,"a");

	fprintf(pFile,"command: \n");
*/
	for(int i=0;i<argc;i++)
	{
//		fprintf(pFile,"argv[%d]=%s \n",i,argv[i]);
	}
	if (strcmp(argv[1], "per-queue") == 0)
	{
		printf("per-queue command is empty!\n");
	}
//	else if (strcmp(argv[1], "queue_num_") == 0)
//	{
//		SetNumQueue(atoi(argv[2]));
//	}
	else
	{
		Tcl& tcl = Tcl::instance();
		if (argc == 2) {
			if (strcmp(argv[1], "reset") == 0) {
				reset();
				return (TCL_OK);
			}
			if (strcmp(argv[1], "early-drop-target") == 0) {
				if (m_Queues->de_drop_ != NULL)
					tcl.resultf("%s", m_Queues->de_drop_->name());
				return (TCL_OK);
			}
			if (strcmp(argv[1], "edrop-trace") == 0) {
				if (m_Queues->EDTrace != NULL) {
					tcl.resultf("%s", m_Queues->EDTrace->name());
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
				tcl.resultf("%s", m_Queues->traceType);
				return (TCL_OK);
			}
			if (strcmp(argv[1], "printstats") == 0) {
				m_Queues->print_summarystats();
				return (TCL_OK);
			}

//			if (strcmp(argv[1], "srcnode") == 0) {
//				NsObject* node = (NsObject*)TclObject::lookup(argv[2]);
//
//				return (TCL_OK);
//			}
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
			if (strcmp(argv[1], "get-size") == 0)
			{
				int queueNum=atoi(argv[2]);
				if(queueNum<0 || queueNum>1)
					return(TCL_ERROR);
				DBGMARK(0,0,"%d",m_Queues[queueNum].qlim_);
				return(TCL_OK);
			}
			if (strcmp(argv[1], "get-threshold") == 0)
			{
				int queueNum=atoi(argv[2]);
				if(queueNum<0 || queueNum>1)
					return(TCL_ERROR);
				DBGMARK(0,0,"%d",m_nThreshold[queueNum]);
				return(TCL_OK);
			}

			// attach a file for variable tracing
			if (strcmp(argv[1], "attach") == 0) {
				int mode;
				const char* id = argv[2];
				m_Queues->tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
				if (m_Queues->tchan_ == 0) {
					tcl.resultf("RED: trace: can't attach %s for writing", id);
					return (TCL_ERROR);
				}
				return (TCL_OK);
			}
			// tell RED about link stats
			if (strcmp(argv[1], "link") == 0) {
				LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
				if (del == 0) {
					tcl.resultf("RED: no LinkDelay object %s",
						argv[2]);
					return(TCL_ERROR);
				}
				// set ptc now
				m_Queues->link_ = del;
				m_Queues->edp_.ptc = m_Queues->link_->bandwidth() /
					(8.0 * m_Queues->edp_.mean_pktsize);
				m_Queues->edp_.delay = m_Queues->link_->delay();
				if (
				  (m_Queues->edp_.q_w <= 0.0 || m_Queues->edp_.th_min_pkts == 0 ||
						  m_Queues->edp_.th_max_pkts == 0))
					initialize_params();
				return (TCL_OK);
			}
			if (strcmp(argv[1], "early-drop-target") == 0) {
				NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
				if (p == 0) {
					tcl.resultf("no object %s", argv[2]);
					return (TCL_ERROR);
				}
				m_Queues->de_drop_ = p;
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
				m_Queues->EDTrace = t;
				if (debug_)
					printf("Ok, Here too too too %d\n", ((Trace *)m_Queues->EDTrace)->type_);
				return (TCL_OK);
			}
			if (!strcmp(argv[1], "packetqueue-attach")) {
				delete m_Queues->q_;
				if (!(m_Queues->q_ = (PacketQueue*) TclObject::lookup(argv[2])))
					return (TCL_ERROR);
				else {
					pq_ = m_Queues->q_;
					return (TCL_OK);
				}
			}
		}
//		fprintf(pFile,"command End: %d\n",ret);
//		fclose(pFile);
		return (Queue::command(argc, argv));
	}
	return ret;
}

void RPQ::bindparams()
{
	/**
	 * Q[0] Params
	 */
	bind_bool("bytes_", &m_Queues->edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &m_Queues->qib_);	    // boolean: q in bytes?
	//	_RENAMED("queue-in-bytes_", "queue_in_bytes_");
	bind("thresh_", &m_Queues->edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_", &m_Queues->edp_.th_min);
	bind("maxthresh_", &m_Queues->edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_", &m_Queues->edp_.th_max);
	bind("mean_pktsize_", &m_Queues->edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_", &m_Queues->edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_", &m_Queues->edp_.q_w);		    // for EWMA
	bind("adaptive_", &m_Queues->edp_.adaptive);          // 1 for adaptive red
	bind("cautious_", &m_Queues->edp_.cautious);          // 1 for cautious marking
	bind("alpha_", &m_Queues->edp_.alpha); 	  	    // adaptive red param
	bind("beta_", &m_Queues->edp_.beta);                  // adaptive red param
	bind("interval_", &m_Queues->edp_.interval);	    // adaptive red param
	bind("feng_adaptive_",&m_Queues->edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_", &m_Queues->edp_.targetdelay);    // target delay
	bind("top_", &m_Queues->edp_.top);		    // maximum for max_p
	bind("bottom_", &m_Queues->edp_.bottom);		    // minimum for max_p
	bind_bool("wait_", &m_Queues->edp_.wait);
	bind("linterm_", &m_Queues->edp_.max_p_inv);
	bind("mark_p_", &m_Queues->edp_.mark_p);
	bind_bool("use_mark_p_", &m_Queues->edp_.use_mark_p);
	bind_bool("setbit_", &m_Queues->edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_", &m_Queues->edp_.gentle);         // increase the packet

	bind_bool("summarystats_", &m_Queues->summarystats_);
	bind_bool("drop_tail_", &m_Queues->drop_tail_);	    // drop last pkt
	//	_RENAMED("drop-tail_", "drop_tail_");

	bind_bool("drop_front_", &m_Queues->drop_front_);	    // drop first pkt
	//	_RENAMED("drop-front_", "drop_front_");

	bind_bool("drop_rand_", &m_Queues->drop_rand_);	    // drop pkt at random
	//	_RENAMED("drop-rand_", "drop_rand_");

	bind_bool("ns1_compat_", &m_Queues->ns1_compat_);	    // ns-1 compatibility
	//	_RENAMED("ns1-compat_", "ns1_compat_");

	bind("ave_", &m_Queues->edv_.v_ave);		    // average queue sie
	bind("prob1_", &m_Queues->edv_.v_prob1);		    // dropping probability
	bind("curq_", &m_Queues->curq_);			    // current queue size
	bind("cur_max_p_", &m_Queues->edv_.cur_max_p);        // current max_p

	/**
	 * Q[1] Params
	 */
	bind_bool("bytes_1", &m_Queues[1].edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_1", &m_Queues[1].qib_);	    // boolean: q in bytes?
	//	_RENAMED("queue-in-bytes_", "queue_in_bytes_");
	bind("thresh_1", &m_Queues[1].edp_.th_min_pkts);		    // minthresh
	bind("thresh_queue_1", &m_Queues[1].edp_.th_min);
	bind("maxthresh_1", &m_Queues[1].edp_.th_max_pkts);	    // maxthresh
	bind("maxthresh_queue_1", &m_Queues[1].edp_.th_max);
	bind("mean_pktsize_1", &m_Queues[1].edp_.mean_pktsize);  // avg pkt size
	bind("idle_pktsize_1", &m_Queues[1].edp_.idle_pktsize);  // avg pkt size for idles
	bind("q_weight_1", &m_Queues[1].edp_.q_w);		    // for EWMA
	bind("adaptive_1", &m_Queues[1].edp_.adaptive);          // 1 for adaptive red
	bind("cautious_1", &m_Queues[1].edp_.cautious);          // 1 for cautious marking
	bind("alpha_1", &m_Queues[1].edp_.alpha); 	  	    // adaptive red param
	bind("beta_1", &m_Queues[1].edp_.beta);                  // adaptive red param
	bind("interval_1", &m_Queues[1].edp_.interval);	    // adaptive red param
	bind("feng_adaptive_1",&m_Queues[1].edp_.feng_adaptive); // adaptive red variant
	bind("targetdelay_1", &m_Queues[1].edp_.targetdelay);    // target delay
	bind("top_1", &m_Queues[1].edp_.top);		    // maximum for max_p
	bind("bottom_1", &m_Queues[1].edp_.bottom);		    // minimum for max_p
	bind_bool("wait_1", &m_Queues[1].edp_.wait);
	bind("linterm_1", &m_Queues[1].edp_.max_p_inv);
	bind("mark_p_1", &m_Queues[1].edp_.mark_p);
	bind_bool("use_mark_p_1", &m_Queues[1].edp_.use_mark_p);
	bind_bool("setbit_1", &m_Queues[1].edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_1", &m_Queues[1].edp_.gentle);         // increase the packet

	bind_bool("summarystats_1", &m_Queues[1].summarystats_);
	bind_bool("drop_tail_1", &m_Queues[1].drop_tail_);	    // drop last pkt
	//	_RENAMED("drop-tail_", "drop_tail_");

	bind_bool("drop_front_1", &m_Queues[1].drop_front_);	    // drop first pkt
	//	_RENAMED("drop-front_", "drop_front_");

	bind_bool("drop_rand_1", &m_Queues[1].drop_rand_);	    // drop pkt at random
	//	_RENAMED("drop-rand_", "drop_rand_");

	bind_bool("ns1_compat_1", &m_Queues[1].ns1_compat_);	    // ns-1 compatibility
	//	_RENAMED("ns1-compat_", "ns1_compat_");

	bind("ave_1", &m_Queues[1].edv_.v_ave);		    // average queue sie
	bind("prob1_1", &m_Queues[1].edv_.v_prob1);		    // dropping probability
	bind("curq_1", &m_Queues[1].curq_);			    // current queue size
	bind("cur_max_p_1", &m_Queues[1].edv_.cur_max_p);        // current max_p

}

void RPQ::reset()
{
	for(int i=0;i<8;i++)
	{
		m_Queues[i].reset();
		m_pfcState[i]=PFC_NORMAL_STS;
	}
}
void RPQ::print_summarystats()
{
	for(int i=0;i<8;i++)
	{
		m_Queues[i].print_summarystats();
	}
}

void RPQ::initialize_params()
{
	for(int i=0;i<8;i++)
	{
		m_Queues[i].initialize_params();
		m_pfcState[i]=PFC_NORMAL_STS;
	}
}
//**********************************************************

void RPQ::enque(Packet* p)
{
	hdr_ip* iph = hdr_ip::access(p);

/*
	FILE* pFile;
	char pName[255];
	sprintf(pName,"/home/student/Desktop/log.txt");

	if(Writecounter)
		pFile = fopen (pName,"a");
	else
		pFile = fopen (pName,"w");
	Writecounter++;
*/
	DBGMARK(DBGPFC,4,"@ %s :Enqueuing...prio:%d\n",this->name(),iph->prio_);
	if (iph->prio_>=0 && iph->prio_<m_nNumQueues)
	{
		DBGMARK(DBGPFC,4,"...\n");
		//if(iph->prio_)DBGMARK(0,0,"m_bPFC:%d now:%f @ %s :Enqueuing...prio:%d pkt=%p qlentgh:%d\n",m_bPFC,NOW,this->name(),iph->prio_,p,m_Queues[iph->prio_].length());
		if(m_bPFC)
		{
			if(m_Queues[iph->prio_].length()<(m_nThreshold[iph->prio_]-m_nMargin) || m_Queues[iph->prio_].length()==0)
			{
				m_pfcState[iph->prio_]=PFC_NORMAL_STS;
			}

			if(m_Queues[iph->prio_].length()>(m_nThreshold[iph->prio_]) && m_pfcState[iph->prio_]!=PFC_PAUSED_STS)
			{
				DBGMARK(DBGPFC,4,"Changing the State to PAUSED...\n");
				m_pfcState[iph->prio_]=PFC_PAUSED_STS;
				//fixme: Calculate Pause Time:
				double pasueTime = 0.000010;//1us		//Random::uniform(0.1);//100ms
				m_pfcTimer[iph->prio_].resched(pasueTime);
				if(iph->prio_)DBGMARK(DBGPFC,4,"\\\\\\\\\\\\name:%s now:%f paused!\n",name(),NOW);
			}
			else
			{
				DBGMARK(DBGPFC,4,"m_Queues[iph->prio_].length():%d\n",m_Queues[iph->prio_].length());
			}
		}
		if(iph->prio_ /*&& m_Queues[iph->prio_].length()>m_nThreshold[iph->prio_]*/)
			DBGMARK(DBGPFC,4,"{****\n now:%f @ %s state:%d Enqueuing...prio:%d pkt=%p qlentgh:%d\n",
					NOW,this->name(),m_pfcState[iph->prio_],iph->prio_,p,m_Queues[iph->prio_].length());
		m_Queues[iph->prio_].enque(p);

		if(iph->prio_ /*&& m_Queues[iph->prio_].length()>m_nThreshold[iph->prio_]*/)
			DBGMARK(DBGPFC,4,"****}\nnow:%f @ %s state:%d Enqueuing...prio:%d pkt=%p qlentgh:%d\n",
					NOW,this->name(),m_pfcState[iph->prio_],iph->prio_,p,m_Queues[iph->prio_].length());

//		fprintf(pFile,"W=%d enque:Q[%d] length:%d src:%d ,dst=%d, blocked=%d \n",Writecounter,iph->prio_,m_Queues[iph->prio_].length(),iph->src_.addr_,iph->dst_.addr_,blocked_);


//    if (m_Queues->length() > qlim_) {
//    	m_Queues->remove(p);
//      drop(p);
//    }
	}
	else
	{
		DBGERROR("Error! INVALID PRIORITY...\n");
		m_Queues[m_nNumQueues-1].enque(p);
//		fprintf(pFile,"OUT of Range Prio enque:Q[%d] length:%d src:%d ,dst=%d, blocked=%d \n",m_nNumQueues-1,m_Queues[m_nNumQueues-1].length(),iph->src_.addr_,iph->dst_.addr_,blocked_);

		//    if (m_Queues->length() > qlim_) {
		//    	m_Queues->remove(p);
		//      drop(p);
		//    }
	}

//	fclose (pFile);
}

Packet* RPQ::pfcdeque(int i)
{
	if(m_bPFC)
	{
		Packet *p;
		if(m_Queues[i].length()!=0)
		{
			int pfc_sts=PFC_NORMAL_STS;
//					DBGMARK(DBGPFC,2,"@%s : Dequeuing...\n",this->name());
			if (m_Queues[i].summarystats_ && &Scheduler::instance() != NULL) {
				Queue::updateStats(m_Queues[i].qib_?m_Queues[i].q_->byteLength():m_Queues[i].q_->length());
			}
			DBGMARK(DBGPFC,4,"...\nm_Queues[i].length():%d",m_Queues[i].length());
			for(int n=0;n<m_Queues[i].length();n++)
			{
//						DBGMARK(DBGPFC,2,"n:%d m_Queues[%d].length():%d...\n",n,i,m_Queues[i].length());
				//To simulate VOQ:
				//If one output queue is paused other pkts should be able to go to other outputs!
				p = m_Queues[i].q_->lookup(n);
				if(p==0)
				{
					DBGMARK(DBGPFC,4,"now:%f @ %s state:%d No More packet .qlentgh:%d\n",
							NOW,this->name(),m_pfcState[i],m_Queues[i].length());
					//All are paused! Or no pkt to send.
					m_Queues[i].SetIdle();
					m_pfcDQTimer[i].resched(0.000001);
					return 0;
				}

				//Check PFC State
				int path = hdr_ip::access(p)->path_;
//						DBGMARK(DBGPFC,1,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p path:%0x qlentgh:%d\n",
//								NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,m_Queues[i].length());
				pfc_sts = GetState(p);
//						DBGMARK(0,0,"GetState: now:%f ",Scheduler::instance().clock());
				if(path!=hdr_ip::access(p)->path_)
				{
					DBGMARK(DBGPFC,4,"now:%f @ %s state:%d n:%d path_change...pkt=%p p->path:%08x path:%08x qlentgh:%d\n",
						NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,path,m_Queues[i].length());
					hdr_ip::access(p)->path_= path;
				}
//						DBGMARK(DBGPFC,1,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p path:%0x qlentgh:%d\n",
//								NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,m_Queues[i].length());

				if(pfc_sts==PFC_NORMAL_STS)
				{
					DBGMARK(DBGPFC,4,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p qlentgh:%d\n",
							NOW,this->name(),m_pfcState[i],n,p,m_Queues[i].length());
					m_Queues[i].q_->remove(p);
					m_Queues[i].idle_=0.0;
//							Packet::free(pTmp);
					m_pfcDQTimer[i].force_cancel();
					return p;
				}
				else
				{
//							Packet::free(pTmp);
					m_pfcDQTimer[i].resched(0.000001);
					DBGMARK(DBGPFC,4,"now:%f name:%s we are paused... My queue length:%d, qlim:%d\n",NOW,name(),m_Queues[i].length(),m_Queues[i].qlim_);
				}
			}
			m_Queues[i].SetIdle();
			return 0;
		}
	//	fprintf(pFile,"EMPTY! deque: length:%d \n",m_Queues[0].length());
	//	fclose (pFile);
		m_Queues[0].SetIdle();
		return 0;
	}
}

Packet* RPQ::deque()
{
	if(m_bPFC)
	{
		Packet *p;
//		DBGMARK(DBGPFC,2,"@%s : Dequeuing...\n",this->name());
		for(int i=0;i<m_nNumQueues;i++)
		{
			if(m_Queues[i].length()!=0)
			{
				int pfc_sts=PFC_NORMAL_STS;
				if(i>0)
				{
//					DBGMARK(DBGPFC,2,"@%s : Dequeuing...\n",this->name());
					if (m_Queues[i].summarystats_ && &Scheduler::instance() != NULL) {
						Queue::updateStats(m_Queues[i].qib_?m_Queues[i].q_->byteLength():m_Queues[i].q_->length());
					}
					DBGMARK(DBGPFC,4,"...\nm_Queues[i].length():%d",m_Queues[i].length());
					for(int n=0;n<m_Queues[i].length();n++)
					{
//						DBGMARK(DBGPFC,2,"n:%d m_Queues[%d].length():%d...\n",n,i,m_Queues[i].length());
						//To simulate VOQ:
						//If one output queue is paused other pkts should be able to go to other outputs!
						p = m_Queues[i].q_->lookup(n);
						if(p==0)
						{
							DBGMARK(DBGPFC,4,"now:%f @ %s state:%d No More packet .qlentgh:%d\n",
									NOW,this->name(),m_pfcState[i],m_Queues[i].length());
							//All are paused! Or no pkt to send.
							m_Queues[i].SetIdle();
							m_pfcDQTimer[i].resched(0.00001);
							return 0;
						}

						//Check PFC State
						int path = hdr_ip::access(p)->path_;
//						DBGMARK(DBGPFC,1,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p path:%0x qlentgh:%d\n",
//								NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,m_Queues[i].length());
						pfc_sts = GetState(p);
//						DBGMARK(0,0,"GetState: now:%f ",Scheduler::instance().clock());
						if(path!=hdr_ip::access(p)->path_)
						{
							DBGMARK(DBGPFC,4,"now:%f @ %s state:%d n:%d path_change...pkt=%p p->path:%08x path:%08x qlentgh:%d\n",
								NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,path,m_Queues[i].length());
							hdr_ip::access(p)->path_= path;
						}
//						DBGMARK(DBGPFC,1,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p path:%0x qlentgh:%d\n",
//								NOW,this->name(),m_pfcState[i],n,p,hdr_ip::access(p)->path_,m_Queues[i].length());

						if(pfc_sts==PFC_NORMAL_STS)
						{
							DBGMARK(DBGPFC,4,"now:%f @ %s state:%d n:%d Dequeuing...pkt=%p qlentgh:%d\n",
									NOW,this->name(),m_pfcState[i],n,p,m_Queues[i].length());
							m_Queues[i].q_->remove(p);
							m_Queues[i].idle_=0.0;
//							Packet::free(pTmp);
							m_pfcDQTimer[i].force_cancel();
							return p;
						}
						else
						{
//							Packet::free(pTmp);
							m_pfcDQTimer[i].resched(0.00001);
							DBGMARK(DBGPFC,4,"now:%f name:%s we are paused... My queue length:%d, qlim:%d\n",NOW,name(),m_Queues[i].length(),m_Queues[i].qlim_);
						}
					}
					m_Queues[i].SetIdle();
					return 0;
				}
				else
				{
					return m_Queues[i].deque();
				}
			}
		}
	//	fprintf(pFile,"EMPTY! deque: length:%d \n",m_Queues[0].length());
	//	fclose (pFile);
		m_Queues[0].SetIdle();
		return 0;
/*
		for(int i=0;i<m_nNumQueues;i++)
		{
			if(m_Queues[i].length()!=0)
			{
				int pfc_sts=PFC_NORMAL_STS;
				if(i>0)
				{
					Packet *p;
					p = m_Queues[i].q_->lookup(0);
					pfc_sts = GetState(p);
				}
				DBGMARK(0,4,"GetState: now:%f ",Scheduler::instance().clock());
				if(pfc_sts==PFC_NORMAL_STS)
				{
					return m_Queues[i].deque();
				}
			}
		}
	//	fprintf(pFile,"EMPTY! deque: length:%d \n",m_Queues[0].length());
	//	fclose (pFile);
		return m_Queues[0].deque();
*/
	}
	else
	{
		Packet *p;
		for(int i=0;i<m_nNumQueues;i++)
		{
			if(m_Queues[i].length()!=0)
			{
	//			fprintf(pFile,"deque:Q[%d] length:%d\n",i,m_Queues[i].length());
	//			fclose (pFile);
				return m_Queues[i].deque();
			}
		}
	//	fprintf(pFile,"EMPTY! deque: length:%d \n",m_Queues[0].length());
	//	fclose (pFile);
		return m_Queues[0].deque();
	}

}

//PFC functionality
int RPQ::CheckState(Packet* p)
{
	if(m_bPFC)
	{
		hdr_ip* iph = hdr_ip::access(p);
		DBGMARK(DBGPFC,4,"@%s: target:%s\n",this->name(),target_->name());
		if (iph->prio_>=0 && iph->prio_<m_nNumQueues)
		{
			if(iph->prio_)
				DBGMARK(DBGPFC,4,"now:%f @ %s state:%d CheckState...pkt=%p qlentgh:%d\n",
					NOW,this->name(),m_pfcState[iph->prio_],p,m_Queues[iph->prio_].length());

			return m_pfcState[iph->prio_];
		}
		DBGERROR("Error! Nothing is found!\n");
	}
	return PFC_NORMAL_STS;
}

void RPQ::CopyPacket(Packet *pTmp,Packet *p)
{
//	hdr_cmn* chOriginal = hdr_cmn::access(p);
//	hdr_cmn* ch = hdr_cmn::access(pTmp);
//
//	ch->uid() = Agent::uidcnt_++;
//	ch->ptype() = chOriginal->ptype_;
//	ch->size() = chOriginal->size_;
//	ch->timestamp() = Scheduler::instance().clock();
//	ch->iface() = UNKN_IFACE.value(); // from packet.h (agent is local)
//	ch->direction() = hdr_cmn::NONE;
//
//	ch->error() = 0;	/* pkt not corrupt to start with */
//
//	hdr_ip* iph = hdr_ip::access(pTmp);
//	hdr_ip* iphOrg = hdr_ip::access(p);
//	iph->saddr() = iphOrg->src_.addr_;
//	iph->sport() = iphOrg->src_.port_;
//	iph->daddr() = iphOrg->dst_.addr_;
//	iph->dport() = iphOrg->dst_.port_;
//
//	iph->flowid() = iphOrg->fid_;
//	iph->prio() = iphOrg->prio_;
//	iph->ttl() = iphOrg->ttl_;
}

//:Test
////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

static class DtRrQueueClass : public TclClass {
public:
        DtRrQueueClass() : TclClass("Queue/DTRR") {}
        TclObject* create(int, const char*const*) {
	         return (new DtRrQueue);
	}
} class_dropt_tail_round_robin;


void DtRrQueue::enque(Packet* p)
{
  hdr_ip* iph = hdr_ip::access(p);

  // if IPv6 priority = 15 enqueue to queue1
  if (iph->prio_ < 1) {
    q1_->enque(p);
    if ((q1_->length() + q2_->length()) > qlim_) {
      q1_->remove(p);
      drop(p);
    }
  }
  else {
    q2_->enque(p);
    if ((q1_->length() + q2_->length()) > qlim_) {
      q2_->remove(p);
      drop(p);
    }
  }
}


Packet* DtRrQueue::deque()
{
  Packet *p;

  if (deq_turn_ == 1) {
    p =  q1_->deque();
    if (p == 0) {
      p = q2_->deque();
      deq_turn_ = 1;
    }
    else {
      deq_turn_ = 2;
    }
  }
  else {
    p =  q2_->deque();
    if (p == 0) {
      p = q1_->deque();
      deq_turn_ = 2;
    }
    else {
      deq_turn_ = 1;
    }
  }

  return (p);
}

