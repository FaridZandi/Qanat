/** ====================================================================
 *
 *  	Copyright 2015,  Abbasloo, All rights reserved.
 *  	Email : ab.@nyu.edu
 *
 *  	New York University
 *
 * ===================================================================== **/

#include "controller.h"
#include <stdlib.h>
SFlow::SFlow(SSortedTableEntry *sEntry)
{
	this->sEntry=sEntry;
}

//SFlow::SFlow()
//{
//	this->sEntry->OrgIndex=0;//parent
//	this->sEntry->pairid=0;
//	this->sEntry->flowid=0;
//	this->sEntry->deadline=0;
//	this->sEntry->start_time=0;
//	this->sEntry->last_time=0;
//	this->sEntry->remaining_size=0;
//	this->sEntry->remaining_deadline=0;
//	this->sEntry->eState = eInit;
//	this->sEntry->ePreState = eInit;
//	this->sEntry->sPath.AggIndex=(-1);
//	this->sEntry->sPath.CoreIndex=(-1);
//
//}

void SFlow::Copy(SFlow other)
{
//	DBGMARK(DBG,8,"this->sEntry:%p....Other.sEntry=%p\n",&this->sEntry,&other.sEntry);
//	this->sEntry->Copy(other.sEntry);
	this->sEntry=other.sEntry;
}

bool SFlow::operator<(const SFlow& other) const
{
//	DBGMARK(DBG,8,"other.Entry:%p\n",other.sEntry);
	if(this->sEntry!=NULL && other.sEntry!=NULL)
	{
		DBGMARK(DBG,8,"this->sEntry->remaining_size:%d....other.sEntry->remaining_size:%d\n",this->sEntry->remaining_size,other.sEntry->remaining_size);
		DBGMARK(DBG,8,"return:%d\n",((*this->sEntry) < (*other.sEntry)));
		return	((*this->sEntry) < (*other.sEntry));
	}
	else
		return	false;
}

bool SFlow::operator!=(const SFlow& other) const
{
	DBGMARK(DBG,8,"\n");
	if(this->sEntry!=NULL && other.sEntry!=NULL)
	{
		DBGMARK(DBG,8,"this->sEntry->remaining_size:%d....other.sEntry->remaining_size:%d\n"
				"sEntry->flowid:%d .... other.sEntry->flowid:%d\n",this->sEntry->remaining_size,other.sEntry->remaining_size
				,this->sEntry->flowid,other.sEntry->flowid);
		DBGMARK(DBG,8,"return:%d\n",((*this->sEntry) != (*other.sEntry)));
		return	((*this->sEntry) != (*other.sEntry));
	}
	else
		return	true;
}

bool SFlow::operator==(const SFlow& other) const
{
	if(this->sEntry!=NULL && other.sEntry!=NULL)
		return	(sEntry->flowid==other.sEntry->flowid);
	if(this->sEntry==NULL && other.sEntry==NULL)
	{
		return true;
	}
	else
		return false;
}

SFlow& SFlow::operator=(const SFlow& other)
{
	if(this != &other)
	{
		Copy(other);
	}
	return *this;
}

SFlowEntry::SFlowEntry(int pairid,int fid,int size,int deadline,
		int src_pod,int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index)
{
	this->pairid=pairid;
	this->flowid=fid;
	this->size=size;
	this->deadline=deadline;
	this->src_pod=src_pod;
	this->src_edg=src_edg;
	this->src_index=src_index;
	this->dst_pod=dst_pod;
	this->dst_edg=dst_edg;
	this->dst_index=dst_index;
	Sorted_Index = 0;//parent
}

bool SFlowEntry::operator<(const SFlowEntry& other) const
{
	return	(flowid<other.flowid)?true:(flowid==other.flowid && pairid<other.pairid)?true:false;
}

void SFlowEntry::Copy(SFlowEntry sEntry)
{
	pairid=sEntry.pairid;
	flowid=sEntry.flowid;
	src_pod=sEntry.src_pod;
	src_edg=sEntry.src_edg;
	src_index=sEntry.src_index;
	dst_pod=sEntry.dst_pod;
	dst_edg=sEntry.dst_edg;
	dst_index=sEntry.dst_index;
	size=sEntry.size;
	deadline=sEntry.deadline;

	Sorted_Index=sEntry.Sorted_Index;
}

bool SFlowEntry::operator!=(const SFlowEntry& other) const
{
	return (flowid != other.flowid);
}

SFlowEntry& SFlowEntry::operator=(const SFlowEntry& other)
{
	if(this != &other)
	{
		Copy(other);
	}
	return *this;
}

void SSortedTableEntry::Copy(SSortedTableEntry sEntry)
{
	pairid=sEntry.pairid;
	flowid=sEntry.flowid;
	start_time=sEntry.start_time;
	last_time=sEntry.last_time;
	remaining_size=sEntry.remaining_size;
	remaining_deadline=sEntry.remaining_deadline;
	deadline=sEntry.deadline;
	eState=sEntry.eState;
	ePreState=sEntry.ePreState;
	OrgIndex=sEntry.OrgIndex;
	sPath.AggIndex=sEntry.sPath.AggIndex;
	sPath.CoreIndex=sEntry.sPath.CoreIndex;
}

bool SSortedTableEntry::operator<(const SSortedTableEntry& other) const
{
	if(deadline>0)
	{
		if(other.deadline==0)
		{
			return true;
		}
		else
		{
			return	(remaining_deadline<other.remaining_deadline)?true:(remaining_deadline>other.remaining_deadline)?false:
					(remaining_deadline==other.remaining_deadline && remaining_size<other.remaining_size)?true:
					(remaining_deadline==other.remaining_deadline && remaining_size>other.remaining_size)?false:
					(flowid<other.flowid)?true:(flowid==other.flowid && pairid<other.pairid)?true:false;
		}
	}
	else
	{
		if(other.deadline>0)
		{
			return false;
		}
		else
		{
			return (remaining_size<other.remaining_size)?true:(remaining_size>other.remaining_size)?false:
					(flowid<other.flowid)?true:(flowid==other.flowid && pairid<other.pairid)?true:false;
		}
	}
}

bool SSortedTableEntry::operator!=(const SSortedTableEntry& other) const
{
	return (!((pairid==other.pairid) && flowid == other.flowid
			&& remaining_size==other.remaining_size && remaining_deadline==other.remaining_deadline
			&& deadline==other.deadline));
}

SSortedTableEntry& SSortedTableEntry::operator=(const SSortedTableEntry& other)
{
	if(this != &other)
	{
		Copy(other);
	}
	return *this;
}

cController::cController()
{
	Init();
}

void cController::Init()
{
	cFlowTree.Initialize(sFlowTable.sTable,MAX_FLOW_NUM,true);
	cSortedTree.Initialize(sSortedTable.sTable,MAX_FLOW_NUM,true);
	NumCore=1;
	NumPod=4;
	NumAggPerPod=MAX_AGG_PER_POD;
	NumCorPerAgg=MAX_CORE_NUM;
	NumEdgPerPod=MAX_EDG_PER_POD;
	NumHostPerEdge=MAX_HOST_PER_EDG;
	HostEdgeBW=1;
	EdgeAggBW=10;
	AggCoreBW=10;

	for(int dir=0;dir<2;dir++)
	{
		for(int i=0;i<MAX_POD_NUM;i++)
		{
			for(int j=0;j<MAX_EDG_PER_POD;j++)
			{
				for(int n=0;n<MAX_HOST_PER_EDG;n++)
				{
//					for(int flow=0;flow<21;flow++)
//					{
//						HostEdgeLink[dir][i][j][n].sFlows.sTable[flow].sEntry = new SSortedTableEntry;
//					}
					HostEdgeLink[dir][i][j][n].cFlowTree.Initialize(HostEdgeLink[dir][i][j][n].sFlows.sTable,MAX_FLOW_PER_LINK,true);
					HostEdgeLink[dir][i][j][n].RBW = HostEdgeBW;
				}
				for(int n=0;n<MAX_AGG_PER_POD;n++)
				{
//					for(int flow=0;flow<21;flow++)
//					{
//						EdgeAggrLink[dir][i][j][n].sFlows.sTable[flow].sEntry = new SSortedTableEntry;
//					}
					EdgeAggrLink[dir][i][j][n].cFlowTree.Initialize(EdgeAggrLink[dir][i][j][n].sFlows.sTable,MAX_FLOW_PER_LINK,true);
					EdgeAggrLink[dir][i][j][n].RBW = EdgeAggBW;

				}
			}
			for(int j=0;j<MAX_CORE_NUM;j++)
			{
				for(int n=0;n<MAX_AGG_PER_POD;n++)
				{
//					for(int flow=0;flow<21;flow++)
//					{
//						AggrCoreLink[dir][i][n][j].sFlows.sTable[flow].sEntry = new SSortedTableEntry;
//					}
					AggrCoreLink[dir][i][n][j].cFlowTree.Initialize(AggrCoreLink[dir][i][n][j].sFlows.sTable,MAX_FLOW_PER_LINK,true);
					AggrCoreLink[dir][i][n][j].RBW = AggCoreBW;
				}
			}
		}
	}
//	InitializeBW();
}

void cController::InitializeBW()
{
	for(int dir=0;dir<2;dir++)
	{
		for(int i=0;i<NumPod;i++)
		{
			for(int j=0;j<NumEdgPerPod;j++)
			{
				for(int n=0;n<NumHostPerEdge;n++)
				{
					HostEdgeLink[dir][i][j][n].RBW = HostEdgeBW;
				}
				for(int n=0;n<NumAggPerPod;n++)
				{
					EdgeAggrLink[dir][i][j][n].RBW = EdgeAggBW;
				}
			}
			for(int j=0;j<NumCore;j++)
			{
				for(int n=0;n<NumAggPerPod;n++)
				{
					AggrCoreLink[dir][i][n][j].RBW = AggCoreBW;
				}
			}
		}
	}
}

void cController::Update()
{
	for(int nIndex=cFlowTree.First();nIndex>0;nIndex=cFlowTree.Next(nIndex))
	{
		int Sorted_Index_ = cFlowTree.Entry(nIndex)->Sorted_Index;
		DBGMARK(DBG,7,"Sorted_Index_:%d\n",Sorted_Index_);
		int changed =0;
		if(cSortedTree.Entry(Sorted_Index_)->deadline)
		{
			cSortedTree.Entry(Sorted_Index_)->remaining_deadline -= (now()-cSortedTree.Entry(Sorted_Index_)->start_time)/1000000;
			if(cSortedTree.Entry(Sorted_Index_)->remaining_deadline<0)
			{
				cSortedTree.Entry(Sorted_Index_)->remaining_deadline =0;
			}
			changed++;
		}
		if(cSortedTree.Entry(Sorted_Index_)->eState==eAllowed)
		{
			//First: Remove it from Path's Links
			RemoveFlow(nIndex);

			Tcl::instance().evalf("%s get-remaining %d %d ", this->name(),cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid);

//			cSortedTree.Entry(Sorted_Index_)->remaining_size -= (now()-cSortedTree.Entry(Sorted_Index_)->last_time)*HostEdgeBW*(1000000000/8);
//			if(cSortedTree.Entry(Sorted_Index_)->remaining_size<=0)
//			{
//				cSortedTree.Entry(Sorted_Index_)->remaining_size=1;
//			}
			changed++;
		}
		DBGMARK(DBG,7,"flowid:%d Remaining deadline:%d Remaining size:%d\n",cSortedTree.Entry(Sorted_Index_)->flowid,cSortedTree.Entry(Sorted_Index_)->remaining_deadline,cSortedTree.Entry(Sorted_Index_)->remaining_size);
		//Update SortedTable
		if(changed)
		{
//			//First: Remove it from Path's Links
//			RemoveFlow(nIndex);

			//Remove it from Sorted List.
			SSortedTableEntry sEntryTmp;
			sEntryTmp.Copy(*cSortedTree.Entry(Sorted_Index_));
			if(cSortedTree.Del(Sorted_Index_)<0)
			{
				DBGMARK(DBG,0,"error!\n");
			}
			//Add the updated one to Sorted List.
			int newIndex = cSortedTree.Add(sEntryTmp);
			if(newIndex<=0)
			{
				DBGERROR("ERROR! Can not add the entry!\n");
				return;
			}
			cFlowTree.Entry(nIndex)->Sorted_Index = newIndex;
			//Add the updated one to Path's Links.
			AddFlow(nIndex);
			DBGMARK(DBG,7,"Sorted_Index_:%d\n",newIndex);
		}
	}
}

void cController::SetRemainingSize(int pairid,int fid,int size,int deadline,int src_pod,int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index,int RemainingSize)
{
	DBGMARK(DBG,2,"time:%lf fid: %d ,size: %d ,deadline: %d ,src_pod: %d ,src_edg: %d ,src_index: %d ,dst_pod: %d ,dst_edg: %d ,dst_index : %d\nRemainingSize:%d\n",now(),fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index,RemainingSize);
	SFlowEntry sEntryTmp(pairid,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
	int nIndex = cFlowTree.Find(sEntryTmp);
	if(nIndex<=0)
	{
		DBGERROR("ERROR! NOT find!\n");
		return;
	}
	int nIndexSorted=cFlowTree.Entry(nIndex)->Sorted_Index;
	if(cSortedTree.Entry(nIndexSorted)->remaining_size >= RemainingSize)
		cSortedTree.Entry(nIndexSorted)->remaining_size = RemainingSize;
}

void cController::NewRequest(int pairid,int fid,int size,int deadline,int src_pod,int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index)
{
	DBGMARK(DBG,2,"fid: %d ,size: %d ,deadline: %d ,src_pod: %d ,src_edg: %d ,src_index: %d ,dst_pod: %d ,dst_edg: %d ,dst_index : %d\n"
			,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
	SFlowEntry sEntryTmp(pairid,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
	int nIndex = cFlowTree.Add(sEntryTmp);
	if(nIndex<=0)
	{
		DBGERROR("ERROR! ADD FAILED!\n");
		return;
	}
	SSortedTableEntry sSortedEntryTmp(pairid,fid,size,deadline,now());
	int nIndexSorted = cSortedTree.Add(sSortedEntryTmp);
	if(nIndexSorted>0)
	{
		cFlowTree.Entry(nIndex)->Sorted_Index=nIndexSorted;
		cSortedTree.Entry(nIndexSorted)->OrgIndex=nIndex;
	}
	else
	{
		DBGMARK(DBG,0,"error!\n");
		return;
	}
//	DBGMARK(DBG,2,"***************\n");
	Schedule(nIndex);
//DBGMARK(DBG,2,"***************\n");
}

void cController::RemoveRequest(int pairid,int fid,int size,int deadline,int src_pod,int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index)
{
	DBGMARK(DBG,2,"fid: %d ,size: %d ,deadline: %d ,src_pod: %d ,src_edg: %d ,src_index: %d ,dst_pod: %d ,dst_edg: %d ,dst_index : %d\n"
			,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
	SFlowEntry sEntryTmp(pairid,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
	int nIndex = cFlowTree.Find(sEntryTmp);
	int NextIndex=0;
	if(nIndex>0)
	{
		if(cFlowTree.Entry(nIndex)->Sorted_Index>0)
		{
			//1st: Remove it from Path's Lkinks:
			RemoveFlow(nIndex);
			NextIndex=cSortedTree.Next(cFlowTree.Entry(nIndex)->Sorted_Index);
			cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index)->eState=eStopped;
			int res= cSortedTree.Del(cFlowTree.Entry(nIndex)->Sorted_Index);
			if(res<0)
			{
				DBGMARK(DBG,0,"Error!\n");
				return;
			}
		}
		if(cFlowTree.Del(nIndex)<0)
		{
			DBGMARK(DBG,0,"Error!\n");
			return;
		}

		Update();
		ReSchedule(cSortedTree.First());
		ShowTable();
	}
	else
	{
		DBGMARK(0,0,"fid: %d ,size: %d ,deadline: %d ,src_pod: %d ,src_edg: %d ,src_index: %d ,dst_pod: %d ,dst_edg: %d ,dst_index : %d\n"
				,fid,size,deadline,src_pod,src_edg,src_index,dst_pod,dst_edg,dst_index);
		DBGERROR("Error,NO ENTRY FOUND!\n");
	}
}

bool cController::CheckAvailableBW(int nIndex)
{
	int Res=0;
	DBGMARK(DBG,7,"FLowID:%d\n",cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index)->flowid);
	SFlow NewFlow(cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index));

	//Start from the dst and come to the src

	//HostEdgeLink
	if(!HostEdgeLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][cFlowTree.Entry(nIndex)->dst_index].cFlowTree.IsEmpty())
	{
		if (HostEdgeLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][cFlowTree.Entry(nIndex)->dst_index].RBW < HostEdgeBW)
		{
			Res = HostEdgeLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][cFlowTree.Entry(nIndex)->dst_index].cFlowTree.FindNext(NewFlow);
			DBGMARK(DBG,7,"Res:%d\n",Res);
			if (Res<=0)
			{
				return false;
			}
		}
	}
	//Check dst edge-aggregation stage
	for (int aggindex=0;aggindex<NumAggPerPod;aggindex++)
	{
		if(		((!EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][aggindex].cFlowTree.IsEmpty()) &&
					(((EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][aggindex].RBW < HostEdgeBW)
							&&(EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][aggindex].cFlowTree.FindNext(NewFlow)>0))
					||(EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][aggindex].RBW >= HostEdgeBW)))
				||(EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][aggindex].cFlowTree.IsEmpty()))
		{
			//Whether there is at least one flow with lower priority:
			//Check dst aggregation-core stage
			for (int AggPort=0;AggPort<NumCorPerAgg;AggPort++)
			{
				int coreindex = AggPort+aggindex*NumCorPerAgg;

				if(		((!AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][aggindex][coreindex].cFlowTree.IsEmpty())&&
							(((AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][aggindex][coreindex].RBW < HostEdgeBW)
								&&(AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][aggindex][coreindex].cFlowTree.FindNext(NewFlow)>0))
							||(AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][aggindex][coreindex].RBW >= HostEdgeBW)))
						||(AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][aggindex][coreindex].cFlowTree.IsEmpty()))
				{
					//srcAggindex is equal to the dstAggindex
					//Check src aggregation-core stage
					if(		((!AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][aggindex][coreindex].cFlowTree.IsEmpty()) &&
									(((AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][aggindex][coreindex].RBW < HostEdgeBW)
											&&(AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][aggindex][coreindex].cFlowTree.FindNext(NewFlow)>0))
									||(AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][aggindex][coreindex].RBW < HostEdgeBW)))
							||(AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][aggindex][coreindex].cFlowTree.IsEmpty()))
					{
						//Check src edge-aggregation stage
						if(		((!EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][aggindex].cFlowTree.IsEmpty()) &&
										(((EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][aggindex].RBW < HostEdgeBW)
												&&(EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][aggindex].cFlowTree.FindNext(NewFlow)>0))
										||	(EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][aggindex].RBW >= HostEdgeBW)))
								||(EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][aggindex].cFlowTree.IsEmpty()))
						{
							//Check src host-edge stage
							if(!HostEdgeLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][cFlowTree.Entry(nIndex)->src_index].cFlowTree.IsEmpty())
							{
								if (HostEdgeLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][cFlowTree.Entry(nIndex)->src_index].RBW < HostEdgeBW)
								{
									Res = HostEdgeLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][cFlowTree.Entry(nIndex)->src_index].cFlowTree.FindNext(NewFlow);
									DBGMARK(DBG,7,"Res:%d",Res);
									if (Res<=0)
									{
										return false;
									}
								}
							}
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool cController::FindPath(int nIndex,list<int> &FinalList,SPath &sPath)
{
	list<int> PreemptionList(7);
	FinalList=PreemptionList;
	bool FirstTime=true;
	bool IsBlocked=true;
	bool IsFound=false;
	SFlow sFlowTmp(cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index));

	int RemBW=65000; //Remaining BW
	SLink *sLink[6];
	int FlowIndex[6];
	int MaxPriorityIndex;
	SSortedTableEntry sMaxPriority;
	sMaxPriority.remaining_size=0xFFFFFFFF;

	for (int AggIndex=0;AggIndex<NumAggPerPod;AggIndex++)
	{
		for (int AggPort=0;AggPort<NumCorPerAgg;AggPort++)
		{
			int CoreIndex = AggPort+AggIndex*NumCorPerAgg;
			IsBlocked=false;
			list<int> LinkFlowList[6];
			//Check All Flows that can be preempted in the Edge-Agg link;

			int RemBWTmp=65000;
			int MaxPriorityIndexTmp;
			/**
			 * Init ///////////////////////////////////////////////////
			 *
			 **/
			//Define Links
			sLink[0]=&HostEdgeLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][cFlowTree.Entry(nIndex)->src_index];
			sLink[1] = &EdgeAggrLink[TX][cFlowTree.Entry(nIndex)->src_pod][cFlowTree.Entry(nIndex)->src_edg][AggIndex];
			sLink[2] = &AggrCoreLink[TX][cFlowTree.Entry(nIndex)->src_pod][AggIndex][CoreIndex];
			sLink[3] = &AggrCoreLink[RX][cFlowTree.Entry(nIndex)->dst_pod][AggIndex][CoreIndex];
			sLink[4] = &EdgeAggrLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][AggIndex];
			sLink[5]=&HostEdgeLink[RX][cFlowTree.Entry(nIndex)->dst_pod][cFlowTree.Entry(nIndex)->dst_edg][cFlowTree.Entry(nIndex)->dst_index];

			DBGMARK(DBG,1,"\n");
			//Create list of lower priority flows in the corresponding path if there is a need for preemption
			for (int nLinkIndex=0;nLinkIndex<6;nLinkIndex++)
			{
				for (int Index=sLink[nLinkIndex]->cFlowTree.FindNext(sFlowTmp);Index>0;Index=sLink[nLinkIndex]->cFlowTree.Next(Index))
				{
					if(sLink[nLinkIndex]->RBW < HostEdgeBW)
						LinkFlowList[nLinkIndex].push_front(sLink[nLinkIndex]->cFlowTree.Entry(Index)->sEntry->OrgIndex);
				}

				if(sLink[nLinkIndex]->RBW < HostEdgeBW && LinkFlowList[nLinkIndex].empty())
				{
					DBGMARK(DBG,7,"This path is not available!\n");
					IsBlocked = true;
				}
			}
			//Go to the next path.
			if(IsBlocked) continue;
			//If at least one path is not blocked: we have a solution!
			IsFound=true;
			DBGMARK(DBG,7,"\n");
			//Create none empty lists. We at least have one none empty list, if we are here!
			list<int> Box[6];
			list<int>::iterator iter[6];
			int BoxNum=0;

			for (int nLinkIndex=0;nLinkIndex<6;nLinkIndex++)
			{
				int RBWTmp_[6];
				//No change in RM BW of this link since the flow will be preempt another flow.
				if(!LinkFlowList[nLinkIndex].empty())
				{
					RBWTmp_[nLinkIndex] = sLink[nLinkIndex]->RBW;
					Box[BoxNum]=LinkFlowList[nLinkIndex];
					iter[BoxNum]=Box[BoxNum].begin();
					BoxNum++;
				}
				//there is enough room. No preemption is needed; therefore, decrease the tmp remaining BW
				else
				{
					RBWTmp_[nLinkIndex] = sLink[nLinkIndex]->RBW - HostEdgeBW;
				}

				if(RemBWTmp>RBWTmp_[nLinkIndex])
				{
					RemBWTmp=RBWTmp_[nLinkIndex];
				}
			}
			for(int BoxIndex=0;BoxIndex<BoxNum;BoxIndex++)
			{
				DBGMARK(DBG,7,"BoxIndex:%d Size:%d\n",BoxIndex,Box[BoxIndex].size());
			}
			DBGMARK(DBG,7,"BoxNum:%d\n",BoxNum);
			bool done=false;
			//Create list of lower priority flows which we can preempt them.
			while(!done)
			{
				list<int> PmListTmp(0);
				for(int BoxIndex=0;BoxIndex<BoxNum;BoxIndex++)
				{
					PmListTmp.push_front(*iter[BoxIndex]);
					DBGMARK(DBG,7,"pushing:(fid:%d)\n",cFlowTree.Entry(*iter[BoxIndex])->flowid);
				}
				for(list<int>::iterator i=PmListTmp.begin();i!=PmListTmp.end();i++)
				{
					DBGMARK(DBG,7,"list:%d",*i);
				}

				DBGMARK(DBG,7,"PreemptionList.size():%d PmListTmp.size():%d\n",PreemptionList.size(),PmListTmp.size());
				PmListTmp.unique();
				//FIXME: here it is better to do a random selection between the lists with equal sizes.
				//With the following approach, we will select the list with lowest priorities (the lowest priorities will be added in final stages).
				if(PreemptionList.size()>=PmListTmp.size())
				{
					PreemptionList=PmListTmp;
				}

				done=true;
				for(int Index=BoxNum-1;Index>=0;Index--)
				{
					iter[Index]++;
					if(iter[Index]!=Box[Index].end())
					{
						for(int k=Index+1;k<BoxNum;k++)
						{
							iter[k]=Box[k].begin();
						}
						done=false;
						break;
					}
					else
					{
						iter[Index]--;
					}
				}
			}
			SSortedTableEntry sflowtmp;
			if(PreemptionList.size())
			{
				for(list<int>::iterator i=PreemptionList.begin();i!=PreemptionList.end();i++)
				{
					//Get Max priority
					if(sflowtmp.remaining_size < (cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->remaining_size))
					{
						sflowtmp=*cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index);
						MaxPriorityIndexTmp = *i;
					}

					DBGMARK(DBG,7,"list:%d",*i);
				}

				if((cSortedTree.Entry(cFlowTree.Entry(MaxPriorityIndexTmp)->Sorted_Index)->remaining_size) < sMaxPriority.remaining_size)
				{
					sMaxPriority=*cSortedTree.Entry(cFlowTree.Entry(MaxPriorityIndexTmp)->Sorted_Index);
				}
			}
			//Calculate Final best flow list (and Path)
//			for(list<int>::iterator i=FinalList.begin();i!=FinalList.end();i++)
//			{
//				DBGMARK(DBG,1,"PreFinal list of flowid:%d \t",cFlowTree.Entry(*i)->flowid);
//			}

			if((FinalList.size()>PreemptionList.size()) || ((FinalList.size()==PreemptionList.size()) && (RemBW>RemBWTmp)))
			{
				DBGMARK(DBG,7,"(RemBW:%d RemBWTmp:%d, AggIndex:%d,CoreIndex:%d)\n",RemBW,RemBWTmp,AggIndex,CoreIndex);
				FinalList=PreemptionList;
				sPath.AggIndex = AggIndex;
				sPath.CoreIndex= CoreIndex;
				//Calculate the Bottleneck BW of the Path
				RemBW=RemBWTmp;
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Now FinalList shows the optimum list of flows that should be preempted in this path.
			//Notice:
			//		: If path is empty (or with enough BW), FinalList is empty.
			//		: This list will be used after invoking this function, to signal "stop" to the ones in the list.
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			for(list<int>::iterator i=FinalList.begin();i!=FinalList.end();i++)
			{
				DBGMARK(DBG,1,"list of flowid:%d remainingSize:%d\t",cFlowTree.Entry(*i)->flowid,cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->remaining_size);
			}
		}
	}

	if(IsFound)
	{
		if(FinalList.size())
		{
			if ((FinalList.size()*cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index)->remaining_size) > sMaxPriority.remaining_size)
			{
				DBGMARK(DBG_MIN_MAX_PRIORITY,1,"Could use preemption but there is minMax issue! :)\n");
				DBGMARK(DBG_MIN_MAX_PRIORITY,1,"MAX[id: %d, rem.size:%d], Num.Preempt:%d, InputPriority:%d \n",sMaxPriority.flowid,sMaxPriority.remaining_size,FinalList.size(),cSortedTree.Entry(cFlowTree.Entry(nIndex)->Sorted_Index)->remaining_size);
				IsFound=false;
			}
		}
	}
		for(list<int>::iterator i=FinalList.begin();i!=FinalList.end();i++)
		{
			DBGMARK(DBG,2,"list of flowid:%d remainingSize:%d\t",cFlowTree.Entry(*i)->flowid,cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->remaining_size);
		}
	DBGMARK(DBG,2,"Isfound:%d\n",IsFound);
	return IsFound;
}

void cController::ManageNewInsertion(int OrgIndex,list<int> FinalList,SPath sPath)
{
	DBGMARK(DBG,6,"***************\n");
	/**Fixme: Should we reschedule the preempted ones?
	 * Just for now: Stop the ones which we should preempt.
	 * We should do this after rescheduling the stopped ones.
	 * We don't like out-of-order delivery!
	 */
	for(list<int>::iterator i=FinalList.begin();i!=FinalList.end();i++)
	{

		cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->ePreState=cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->eState;
		//Remove it from its path's links.
		RemoveFlow(*i);

		//Fixme: Should we reschedule the preempted ones?
		//To skip it in rescheduling. Just for now...
		cSortedTree.Entry(cFlowTree.Entry(*i)->Sorted_Index)->eState=eWillBeStopped;
		SendSignal(cFlowTree.Entry(*i)->Sorted_Index,eStopped);
	}
	int nIndex= cFlowTree.Entry(OrgIndex)->Sorted_Index;
	cSortedTree.Entry(nIndex)->ePreState=cSortedTree.Entry(nIndex)->eState;
	cSortedTree.Entry(nIndex)->eState=eAllowed;
	cSortedTree.Entry(nIndex)->last_time=now();
	cSortedTree.Entry(nIndex)->sPath=sPath;
	AddFlow(OrgIndex);
	SendSignal(nIndex,eAllowed);
}

void cController::ReSchedule(int ind)
{
	DBGMARK(DBG,2,"Starting...\n");
	for(int nIndex=ind;nIndex>0;nIndex=cSortedTree.Next(nIndex))
//	for(int nIndex=cSortedTree.First();nIndex>0;nIndex=cSortedTree.Next(nIndex))
	{
		if(cSortedTree.Entry(nIndex)->eState==eStopped)
		{
			bool Res = CheckAvailableBW(cSortedTree.Entry(nIndex)->OrgIndex);
			if (Res)
			{
				list<int> FinalList(7);
				SPath sPath;
				Res = FindPath(cSortedTree.Entry(nIndex)->OrgIndex,FinalList,sPath);
				if(Res)
				{
					DBGMARK(DBG,1,"Accepting this flow (id:%d,state:%d)...\n",cSortedTree.Entry(nIndex)->flowid,cSortedTree.Entry(nIndex)->eState);
					ManageNewInsertion(cSortedTree.Entry(nIndex)->OrgIndex,FinalList,sPath);
					DBGMARK(DBG,7,"\n");
				}
			}
		}
	}
	for(int nIndex=ind;nIndex>0;nIndex=cSortedTree.Next(nIndex))
//	for(int nIndex=cSortedTree.First();nIndex>0;nIndex=cSortedTree.Next(nIndex))
	{
		if(cSortedTree.Entry(nIndex)->eState==eWillBeStopped)
		{
			cSortedTree.Entry(nIndex)->eState=eStopped;
		}
	}
	DBGMARK(DBG,2,"end.\n");
}

void cController::Schedule(int index)
{
	DBGMARK(DBG,2,"\n");

	Update();

	int nIndex=cFlowTree.Entry(index)->Sorted_Index;

	int Res = CheckAvailableBW(index);

	if (Res==false)
	{
		DBGMARK(DBG,1,"We can not accept this flow (%d, state:%d) yet! Wait...\n",cSortedTree.Entry(nIndex)->flowid,cSortedTree.Entry(nIndex)->eState);
		if(cSortedTree.Entry(nIndex)->eState!=eStopped)
		{
			cSortedTree.Entry(nIndex)->ePreState=cSortedTree.Entry(nIndex)->eState;
			cSortedTree.Entry(nIndex)->eState=eStopped;
			SendSignal(nIndex,eStopped);
		}
	}
	else
	{
		DBGMARK(DBG,1,"Accepting this flow (id:%d,state:%d)...\n",cSortedTree.Entry(nIndex)->flowid,cSortedTree.Entry(nIndex)->eState);
		list<int> FinalList(7);
		SPath sPath;
		Res=FindPath(index,FinalList,sPath);
		if(Res)
		{
			DBGMARK(DBG,1,"\n");
			ManageNewInsertion(index,FinalList,sPath);
			DBGMARK(DBG,1,"\n");
		}
		else
		{
			DBGMARK(DBG,1,"We can not accept this flow (%d, state:%d) yet! Wait...\n",cSortedTree.Entry(nIndex)->flowid,cSortedTree.Entry(nIndex)->eState);
			if(cSortedTree.Entry(nIndex)->eState!=eStopped)
			{
				cSortedTree.Entry(nIndex)->ePreState=cSortedTree.Entry(nIndex)->eState;
				cSortedTree.Entry(nIndex)->eState=eStopped;
				SendSignal(nIndex,eStopped);
			}
		}
		DBGMARK(DBG,1,"Done!\n");
	}
	//Reschedule teh flows with State==eStopped & Stop All flows with State==eWillBeStopped
//	int nextIndex=cSortedTree.Next(cFlowTree.Entry(index)->Sorted_Index);
	int nextIndex=cSortedTree.First();
	if(Res && nextIndex>0)
		ReSchedule(nextIndex);
	DBGMARK(DBG,5,"***************pair:%d flowid:%d %d\n",cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid
			,cSortedTree.Entry(nIndex)->eState);
}

void cController::RemoveFlow(int OrgIndex)
{
	if(cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->eState!=eAllowed)
	{
		DBGMARK(DBG,0,"ERROR! This Flow is not eAllowed.\n");
		return;
	}
	int AggIndex = cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.AggIndex;
	int CoreIndex= cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.CoreIndex;
	SLink *sLink[6];

	SFlow FlowTmp(cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index));

	sLink[0]=&HostEdgeLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][cFlowTree.Entry(OrgIndex)->src_edg][cFlowTree.Entry(OrgIndex)->src_index];
	sLink[1] = &EdgeAggrLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][cFlowTree.Entry(OrgIndex)->src_edg][AggIndex];
	sLink[2] = &AggrCoreLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][AggIndex][CoreIndex];
	sLink[3] = &AggrCoreLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][AggIndex][CoreIndex];
	sLink[4] = &EdgeAggrLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][cFlowTree.Entry(OrgIndex)->dst_edg][AggIndex];
	sLink[5]=&HostEdgeLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][cFlowTree.Entry(OrgIndex)->dst_edg][cFlowTree.Entry(OrgIndex)->dst_index];

	DBGMARK(DBG,3,"now:%f Deleting Flow id:%d state:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
			,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->flowid,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->eState
			,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->remaining_size,
			cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.AggIndex,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.CoreIndex);

	DBGMARK(DBG,3,"now:%f Deleting Flow id:%d state:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
			,FlowTmp.sEntry->flowid,FlowTmp.sEntry->eState,FlowTmp.sEntry->remaining_size,
			FlowTmp.sEntry->sPath.AggIndex,FlowTmp.sEntry->sPath.CoreIndex);

	for(int LinkIndex=0;LinkIndex<6;LinkIndex++)
	{
		DBGMARK(DBG,5,"********************** LinkIndex:%d ... id:%d ... rem_size:%d\n",LinkIndex, FlowTmp.sEntry->flowid,FlowTmp.sEntry->remaining_size);
		int nIndex=sLink[LinkIndex]->cFlowTree.Del(FlowTmp);
		DBGMARK(DBG,5,"********************** LinkIndex:%d ... id:%d ... rem_size:%d\n",LinkIndex, FlowTmp.sEntry->flowid,FlowTmp.sEntry->remaining_size);
		//		int nRes=sLink[LinkIndex]->cFlowTree.Del(FlowTmp);
		if(nIndex>=0)
		{
			sLink[LinkIndex]->RBW+=HostEdgeBW;
			if(sLink[LinkIndex]->RBW > 10)
			{
				DBGMARK(DBG,0,"{(%d,%d,%d) -> (%d,%d,%d)  @ sLink[%d]=[agg:%d][core:%d]->RBW:%d} \n",
						cFlowTree.Entry(OrgIndex)->src_pod,cFlowTree.Entry(OrgIndex)->src_edg,cFlowTree.Entry(OrgIndex)->src_index
						,cFlowTree.Entry(OrgIndex)->dst_pod,cFlowTree.Entry(OrgIndex)->dst_edg,cFlowTree.Entry(OrgIndex)->dst_index
						,LinkIndex,AggIndex,CoreIndex,sLink[LinkIndex]->RBW);
				ShowLinksFlows(sLink[LinkIndex]);
			}
		}
		else
		{
			DBGMARK(DBG,0,"{(%d,%d,%d) -> (%d,%d,%d)  @ sLink[%d]=[agg:%d][core:%d]->RBW:%d} \n",
					cFlowTree.Entry(OrgIndex)->src_pod,cFlowTree.Entry(OrgIndex)->src_edg,cFlowTree.Entry(OrgIndex)->src_index
					,cFlowTree.Entry(OrgIndex)->dst_pod,cFlowTree.Entry(OrgIndex)->dst_edg,cFlowTree.Entry(OrgIndex)->dst_index
					,LinkIndex,AggIndex,CoreIndex,sLink[LinkIndex]->RBW);
			ShowLinksFlows(sLink[LinkIndex]);

			DBGMARK(DBG,0,"Error! Can not Find it: nIndex{%d}\n",nIndex);
			DBGERROR("ABORTTING!\n");
			abort();
		}
	}
}

void cController::AddFlow(int OrgIndex)
{
	if(cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->eState!=eAllowed)
	{
		DBGMARK(DBG,0,"ERROR! This Flow is not allowed.\n");
		return;
	}

	int AggIndex = cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.AggIndex;
	int CoreIndex= cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.CoreIndex;
	SLink *sLink[6];

	SFlow FlowTmp(cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index));

	sLink[0]=&HostEdgeLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][cFlowTree.Entry(OrgIndex)->src_edg][cFlowTree.Entry(OrgIndex)->src_index];
	sLink[1] = &EdgeAggrLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][cFlowTree.Entry(OrgIndex)->src_edg][AggIndex];
	sLink[2] = &AggrCoreLink[TX][cFlowTree.Entry(OrgIndex)->src_pod][AggIndex][CoreIndex];
	sLink[3] = &AggrCoreLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][AggIndex][CoreIndex];
	sLink[4] = &EdgeAggrLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][cFlowTree.Entry(OrgIndex)->dst_edg][AggIndex];
	sLink[5]=&HostEdgeLink[RX][cFlowTree.Entry(OrgIndex)->dst_pod][cFlowTree.Entry(OrgIndex)->dst_edg][cFlowTree.Entry(OrgIndex)->dst_index];

	DBGMARK(DBG,3,"now:%f Adding Flow id:%d state:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
			,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->flowid,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->eState
			,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->remaining_size,
			cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.AggIndex,cSortedTree.Entry(cFlowTree.Entry(OrgIndex)->Sorted_Index)->sPath.CoreIndex);

	DBGMARK(DBG,3,"now:%f Deleting Flow id:%d state:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
			,FlowTmp.sEntry->flowid,FlowTmp.sEntry->eState,FlowTmp.sEntry->remaining_size,
			FlowTmp.sEntry->sPath.AggIndex,FlowTmp.sEntry->sPath.CoreIndex);

	for(int LinkIndex=0;LinkIndex<6;LinkIndex++)
	{
		DBGMARK(DBG,5,"********************** LinkIndex:%d ... id:%d ... rem_size:%d\n",LinkIndex, FlowTmp.sEntry->flowid,FlowTmp.sEntry->remaining_size);
		int res= sLink[LinkIndex]->cFlowTree.Add(FlowTmp);
		DBGMARK(DBG,5,"********************** LinkIndex:%d ... id:%d ... rem_size:%d\n",LinkIndex, FlowTmp.sEntry->flowid,FlowTmp.sEntry->remaining_size);
		//FIXME: We should not write following lines! Somewhere, something is wrong! :(
		if(res>=0)
		{
			sLink[LinkIndex]->RBW-=HostEdgeBW;
		}
		else
		{
			DBGMARK(DBG,0,"{(%d,%d,%d) -> (%d,%d,%d)  @ sLink[%d]=[agg:%d][core:%d]->RBW:%d} \n",
					cFlowTree.Entry(OrgIndex)->src_pod,cFlowTree.Entry(OrgIndex)->src_edg,cFlowTree.Entry(OrgIndex)->src_index
					,cFlowTree.Entry(OrgIndex)->dst_pod,cFlowTree.Entry(OrgIndex)->dst_edg,cFlowTree.Entry(OrgIndex)->dst_index
					,LinkIndex,AggIndex,CoreIndex,sLink[LinkIndex]->RBW);
			ShowLinksFlows(sLink[LinkIndex]);
//			sLink[LinkIndex]->RBW=0;
			DBGERROR("ABORTTING!\n");
			abort();
		}
		//Show table:
//		for(int i=sLink[LinkIndex]->cFlowTree.First();i>0;i=sLink[LinkIndex]->cFlowTree.Next(i))
//		{
//			DBGMARK(DBG,1,"Table:\nLink:%d\tFlowID:%dRem Size:%d\n",LinkIndex,sLink[LinkIndex]->cFlowTree.Entry(i)->sEntry->flowid,sLink[LinkIndex]->cFlowTree.Entry(i)->sEntry->remaining_size);
//		}

	}
}

void cController::ShowLinksFlows(SLink *sLink)
{
	DBGMARK(DBG,2,"On this Link:\n");
	for(int i=sLink->cFlowTree.First();i>0;i=sLink->cFlowTree.Next(i))
	{
		DBGMARK(DBG,0,"now:%f id:%d state:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
				,sLink->cFlowTree.Entry(i)->sEntry->flowid,sLink->cFlowTree.Entry(i)->sEntry->eState
				,sLink->cFlowTree.Entry(i)->sEntry->remaining_size,
				sLink->cFlowTree.Entry(i)->sEntry->sPath.AggIndex,sLink->cFlowTree.Entry(i)->sEntry->sPath.CoreIndex);

		int OrgIndex=sLink->cFlowTree.Entry(i)->sEntry->OrgIndex;
		DBGMARK(DBG,0,"\t\t\t\t{(%d,%d,%d) -> (%d,%d,%d)} \n",
					cFlowTree.Entry(OrgIndex)->src_pod,cFlowTree.Entry(OrgIndex)->src_edg,cFlowTree.Entry(OrgIndex)->src_index
					,cFlowTree.Entry(OrgIndex)->dst_pod,cFlowTree.Entry(OrgIndex)->dst_edg,cFlowTree.Entry(OrgIndex)->dst_index);

	}
}

void cController::ShowTable()
{
	for(int nIndex=cSortedTree.First();nIndex>0;nIndex=cSortedTree.Next(nIndex))
	{

		if(cSortedTree.Entry(nIndex)->eState==eAllowed)
		{
			DBGMARK(DBG,1,"now:%f start:%lf id:%d state:%d prestate:%d remainingSize:%d Path:{Agg:%d,Core:%d}\n",now()
					,cSortedTree.Entry(nIndex)->start_time,cSortedTree.Entry(nIndex)->flowid,cSortedTree.Entry(nIndex)->eState,cSortedTree.Entry(nIndex)->ePreState
					,cSortedTree.Entry(nIndex)->remaining_size,cSortedTree.Entry(nIndex)->sPath.AggIndex,cSortedTree.Entry(nIndex)->sPath.CoreIndex);
			int OrgIndex=cSortedTree.Entry(nIndex)->OrgIndex;
			DBGMARK(DBG,2,"\t\t\t\t{(%d,%d,%d) -> (%d,%d,%d)} \n",
						cFlowTree.Entry(OrgIndex)->src_pod,cFlowTree.Entry(OrgIndex)->src_edg,cFlowTree.Entry(OrgIndex)->src_index
						,cFlowTree.Entry(OrgIndex)->dst_pod,cFlowTree.Entry(OrgIndex)->dst_edg,cFlowTree.Entry(OrgIndex)->dst_index);
		}
	}
}

void cController::SendSignal(int nIndex,EFlowState eState)
{
	if (eState==eAllowed)
	{
		int nPath = 0;
		int AggrAddrBase=((0x00) + NumAggPerPod*cFlowTree.Entry(cSortedTree.Entry(nIndex)->OrgIndex)->src_pod);
		int AggAddress = AggrAddrBase+cSortedTree.Entry(nIndex)->sPath.AggIndex;
		int CoreAddrBase=NumAggPerPod*NumPod;
		int CoreAddress=CoreAddrBase+cSortedTree.Entry(nIndex)->sPath.CoreIndex;

		nPath = (AggAddress & 0x7F);
		DBGMARK(DBG,7,"nPath:%08x\n",nPath);
		nPath <<=8;
		DBGMARK(DBG,7,"nPath:%08x\n",nPath);
		nPath |= (CoreAddress & 0x7F);
		DBGMARK(DBG,7,"nPath:%08x\n",nPath);
		nPath <<=16;
		nPath |= 0xFFFF;
		DBGMARK(DBG,7,"nPath:%08x\n",nPath);

		DBGMARK(DBG,5,"#####     %s signal %d %d %s %d", this->name(),cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid,"Go",nPath);
		Tcl::instance().evalf("%s signal %d %d %s %d", this->name(),cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid,"Go",nPath);
	}
	else
	{
		DBGMARK(DBG,5,"#####     %s signal %d %d %s #####\n",this->name(),cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid,"Stop");
		Tcl::instance().evalf("%s signal %d %d %s 0", this->name(),cSortedTree.Entry(nIndex)->pairid,cSortedTree.Entry(nIndex)->flowid,"Stop");
	}

}

int cController::command(int argc, const char*const* argv)
{
	DBGMARK(DBG,7,"argc=%d\n",argc);
	if (argc == 13)
	{	DBGMARK(DBG,7,"\n");
		if (strcmp(argv[1], "set_rem_size") == 0)
		{
				DBGMARK(DBG,5,"set_rem_size!\n");
				SetRemainingSize(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7])
						,atoi(argv[8]),atoi(argv[9]),atoi(argv[10]),atoi(argv[11]),atoi(argv[12]));
				return(TCL_OK);
		}
	}
	else if (argc == 12) {
		if (strcmp(argv[1], "request") == 0) {
				DBGMARK(DBG,5,"new incoming request!\n");
				NewRequest(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7])
						,atoi(argv[8]),atoi(argv[9]),atoi(argv[10]),atoi(argv[11]));
				return(TCL_OK);
		}
		if (strcmp(argv[1], "rm-request") == 0) {
				DBGMARK(DBG,5,"Removing request\n");
				RemoveRequest(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7])
						,atoi(argv[8]),atoi(argv[9]),atoi(argv[10]),atoi(argv[11]));
				return(TCL_OK);
		}
	}
	else if (argc ==2)
	{
		if (strcmp(argv[1], "showtabel") == 0)
		{
			ShowTable();
			return(TCL_OK);
		}
	}
	else if (argc ==8)
	{
		if (strcmp(argv[1], "settop") == 0)
		{
			SetTop(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7]));
			return(TCL_OK);
		}
	}
	else if (argc ==5)
	{
		if (strcmp(argv[1], "setbw") == 0)
		{
			SetBW(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));
			return(TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

void cController::SetTop(int NumHostPerEdge, int NumEdgPerPod, int NumAggPerPod, int NumCorPerAgg,int NumPod, int NumCore)
{
	this->NumCore=NumCore;
	this->NumPod=NumPod;
	this->NumAggPerPod=NumAggPerPod;
	this->NumCorPerAgg=NumCorPerAgg;
	this->NumEdgPerPod=NumEdgPerPod;
	this->NumHostPerEdge=NumHostPerEdge;
}

void cController::SetBW(int HostEdgeBW, int EdgeAggBW, int AggCoreBW)
{
	this->HostEdgeBW=HostEdgeBW;
	this->EdgeAggBW=EdgeAggBW;
	this->AggCoreBW=AggCoreBW;
	InitializeBW();
}


//int cController::SetPath(int *pPath, int nSize)
//{
//	int nPath = 0;
//	int AggrAddrBase=0;
//	int CoreAddrBase=NumAggPerPod*NumPod;
//	DBGMARK(DBG,1,"CoreAddrBase:%08x\n",CoreAddrBase);
//	nPath = (AggrAddrBase & 0x7F + NumAggPerPod*cFlowTree.Entry(cSortedTree.Entry(nIndex)->OrgIndex)->src_pod);
//	DBGMARK(DBG,1,"nPath:%08x\n",nPath);
//	nPath <<=8;
//	DBGMARK(DBG,1,"nPath:%08x\n",nPath);
//	nPath |= ((CoreAddrBase+1) & 0x7F );
//	DBGMARK(DBG,1,"nPath:%08x\n",nPath);
//	nPath <<=16;
//	nPath |= 0xFFFF;
//	DBGMARK(DBG,1,"nPath:%08x\n",nPath);
//}
