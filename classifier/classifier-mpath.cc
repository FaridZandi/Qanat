/* -*- Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t
              -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: classifier-mpath.cc,v 1.10 2005/08/25 18:58:01 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-mpath.cc,v 1.10 2005/08/25 18:58:01 johnh Exp $ (USC/ISI)";
#endif

#include "classifier.h"
#include "ip.h"
#define MAX_SLOT 6500

//#define DBGPFC 2

class MultiPathForwarder : public Classifier {
public:
	MultiPathForwarder() : ns_(0), nodeid_(0), nodetype_(0), perflow_(0), checkpathid_(0) {
		bind("nodeid_", &nodeid_); 
		bind("nodetype_", &nodetype_);
		bind("perflow_", &perflow_);
		bind("checkpathid_", &checkpathid_);
	} 
	virtual int classify(Packet* p) {
      		int cl;
		hdr_ip* h = hdr_ip::access(p);
		/**
		 * : deterministic routing support.
		 * this implementation only works if the multipath option are one after each other,
		 * and if they is no single path route before them
		 */

		if(h->prio_)DBGMARK(DBGCLS,1,"now:%f, pkt:%p @ {name:%s id:%d}:********** h->path_enable():%d \n",NOW,p,this->name(),nodeid(),h->path_enable());
		if(h->path_enable())
		{
			/*
			 * FIXME: We here considered that we are using FatTree (pfabric style)
			 * End Host does not have multipath option. Only Edge and aggregate switch have.
			 */
			//Circumventing End Host!
			if(maxslot_>=1)
			{
				int nexthoptmp=0;
				int mask = 0x80;
				//Use the 1st unused option.
				for(int i=3;i>=0;i--)
				{
					nexthoptmp = h->path();
					DBGMARK(DBGCLS,1,"now:%f, pkt:%p @ {id:%d}: nexthoptmp:%08x\n",NOW,p,nodeid(),nexthoptmp);
					nexthoptmp >>= (i*8);
					nexthoptmp = (nexthoptmp & 0xFF);
					DBGMARK(DBGCLS,4,"********** nexthoptmp:%08x\n",nexthoptmp);
					if((mask & ~nexthoptmp))
					{
						for(int j=0;j<=maxslot_;j++)
						{
							if(nexthoptmp==toNode_[j])
							{
								cl = j;
								h->path()|= (0x80)<<(i*8);
								DBGMARK(DBGCLS,1,"@ {id:%d}: {src,dst:%d,%d}Slot is found! %d\n",nodeid(),cl,h->src().addr_,h->dst().addr_);
								return cl;
							}
							DBGMARK(DBGCLS,4,"@ {id:%d}: No Slot is found! nexthoptmp:%d maxslot_:%d \n",
									nodeid(),nexthoptmp,maxslot_);
						}
					}
				}
			}
			else
			{
				DBGMARK(DBGCLS,4,"@ {id:%d}: Endhost?! maxslot_:%d\n",nodeid(),maxslot_);
			}
		}

		// Mohammad: multipath support
		// fprintf(stdout, "perflow_ = %d, rcv packet in classifier\n", perflow_);
		if (perflow_ || checkpathid_) {		  
		  /*if (h->flowid() >= 10000000) {
		  	int fail = ns_;
			do {
			  cl = ns_++;
			  ns_ %= (maxslot_ + 1);
			} while (slot_[cl] == 0 && ns_ != fail);		
			return cl;
			}*/
		  
			struct hkey {
				int nodeid;
					nsaddr_t src, dst;
				int fid;
			};
			struct hkey buf_;
			buf_.nodeid = nodeid_;
			buf_.src = mshift(h->saddr());
			buf_.dst = mshift(h->daddr());
			buf_.fid = h->flowid();
			/*if (checkpathid_)
				buf_.prio = h->prio();
			else
			buf_.prio = 0;*/
			char* bufString = (char*) &buf_;
			int length = sizeof(hkey);

			unsigned int ms_ = (unsigned int) HashString(bufString, length);
			if (checkpathid_) {
				int pathNum = h->prio();
				int pathDig;
				for (int i = 0; i < nodetype_; i++) {
					pathDig = pathNum % 8;
					pathNum /= 8;
				}
				//printf("%d: %d->%d\n", nodetype_, h->prio(), pathDig);
				ms_ += h->prio(); //pathDig;
			}
			ms_ %= (maxslot_ + 1);
			//printf("nodeid = %d, pri = %d, ms = %d\n", nodeid_, buf_.prio, ms_);
			int fail = ms_;
			do {
				cl = ms_++;
				ms_ %= (maxslot_ + 1);
			} while (slot_[cl] == 0 && ms_ != fail);
			//printf("nodeid = %d, pri = %d, cl = %d\n", nodeid_, h->prio(), cl);
		}
		else {
			//hdr_ip* h = hdr_ip::access(p);
			//if (h->flowid() == 45) {
			//cl = h->prio() % (maxslot_ + 1);
			//}
			//else {
			int fail = ns_;
			do {
				cl = ns_++;
				ns_ %= (maxslot_ + 1);
			} while (slot_[cl] == 0 && ns_ != fail);
		}
		//}
		return cl;
	}
	int  nodeid(){return nodeid_;};
	virtual int install_next(NsObject *node,int ntoNodeid)
	{
		if(ntoNodeid==(-1))
		{
			DBGERROR("ERRROR! Invalid toNodeid\n");
			return 0;
		}
		DBGPRINT(DBGCLS,1,"ntoNodeid:%d",ntoNodeid);
		int slot = maxslot_ + 1;
		install(slot, node,ntoNodeid);
		return (slot);
	}
	virtual void install(int slot, NsObject* p,int ntoNodeid)
	{
		if(ntoNodeid==(-1))
		{
			DBGERROR("ERRROR! Invalid toNodeid\n");
			return;
		}
		if(slot>=MAX_SLOT)
		{
			DBGERROR("ERRROR! slot:%d is more than MAX_SLOT:%d\n",slot,MAX_SLOT);
			return;
		}
		if (slot >= nslot_)
			alloc(slot);
		slot_[slot] = p;
		toNode_[slot]=ntoNodeid;
		if (slot >= maxslot_)
			maxslot_ = slot;
	}
	virtual void alloc(int slot)
	{
		NsObject** old = slot_;
		int n = nslot_;
		if (old == 0)
		    {
			if (nsize_ != 0) {
				//printf("classifier %x set to %d....%dth visit\n", this, nsize_, i++);
				nslot_ = nsize_;
			}
			else {
				//printf("classifier %x set to 32....%dth visit\n", this, j++);
				nslot_ = 32;
			}
		    }
		while (nslot_ <= slot)
			nslot_ <<= 1;
		slot_ = new NsObject*[nslot_];
		memset(slot_, 0, nslot_ * sizeof(NsObject*));
		for (int i = 0; i < n; ++i)
		{
			slot_[i] = old[i];
		}
		delete [] old;
	}
	virtual int command(int argc, const char*const* argv)
	{
		Tcl& tcl = Tcl::instance();
		/*
		 * $classifier installNext $Connector $toNode_id
		 */
		if (strcmp(argv[1], "installNext") == 0)
		{
			if(argc == 4)
			{
				//int slot = maxslot_ + 1;
				NsObject* node = (NsObject*)TclObject::lookup(argv[2]);
				if (node == NULL) {
					tcl.resultf("Classifier::installNext attempt "
				"to install non-object %s into classifier", argv[2]);
					return TCL_ERROR;
				};
				int ntoNode = atoi(argv[3]);
				DBGMARK(DBGCLS,1,"ntoNode_:%d\n",ntoNode);
				int slot = install_next(node,ntoNode);
				tcl.resultf("%u", slot);

//				tcl.evalf("[%s set toNode_]",node->name());
//				const char* result = tcl.result();
//				DBGMARK(DBGCLS,1,"classifier_name:%s link:%s, node:%s slot:%d\n",this->name(),result, node->name(),slot);
				return TCL_OK;
			}
			else
			{
				DBGMARK(DBGCLS,1,"ERROR!, not enough params for installnext!\n");
				return TCL_ERROR;
			}
		}
		else if (strcmp(argv[1], "pfcmessage") == 0)
		{
			if(argc == 5)
			{
				int ntoNodeid = atoi(argv[2]);
				int npriority = atoi(argv[3]);
				int nduration = atoi(argv[4]);
				DBGMARK(DBGPFC,1,"ntoNodeid_:%d priority:%d duration:%d\n",ntoNodeid,npriority,nduration);
				pfcmessage(ntoNodeid,npriority,nduration);
				return TCL_OK;
			}
		}

		//:

		/**
		 * : We do not need to overload the "install" command
		 * We always call installnext for multipath
		 */
		return (Classifier::command(argc, argv));
	}
	virtual void pfcmessage(int ntoNodeid,int npriority,int nduration)
	{
		DBGPRINT(DBGPFC,2,"Message Received!\n");
		DBGMARK(DBGPFC,2,"ntoNodeid_:%d priority:%d duration:%d\n",ntoNodeid,npriority,nduration);
		/**
		 *	CODE:
		 */

	}
	virtual bool IsMultiPathForwarder(NsObject*node)
	{
		MultiPathForwarder* clf2 =dynamic_cast<MultiPathForwarder*>(node);
		DBGMARK(DBGPFC,4,"************************* Checking %s ...:(clf2!=0):%d ",node->name(),(clf2!=0));
		return (clf2!=0);
	}
private:
	// fixme:MAKE it Dynamic!
	/*
	 * The dynamic version does not work properly!
	 */
	int toNode_[MAX_SLOT];

	int ns_;
	// Mohamamd: adding support for perflow multipath
	int nodeid_;
	int nodetype_;
	int perflow_;
	int checkpathid_;

	static unsigned int
	HashString(register const char *bytes,int length)
	{
		register unsigned int result;
		register int i;

		result = 0;
		for (i = 0;  i < length;  i++) {
			result += (result<<3) + *bytes++;
		}
		return result;
	}


};

static class MultiPathClass : public TclClass {
public:
	MultiPathClass() : TclClass("Classifier/MultiPath") {} 
	TclObject* create(int, const char*const*) {
		return (new MultiPathForwarder());
	}
} class_multipath;
