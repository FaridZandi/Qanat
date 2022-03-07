/** ====================================================================
 *
 *  	Copyright 2015,  Abbasloo, All rights reserved.
 *  	Email : ab.@nyu.edu
 *
 *  	New York University
 *
 * ===================================================================== **/

#ifndef NS_2_34_COMMON_CONTROLLER_H_
#define NS_2_34_COMMON_CONTROLLER_H_

#include "common.h"
#include "RBTree.h"
#include <tclcl.h>
#include "scheduler.h"
#include <list>

enum EFlowState
{
	eInit=0,
	eStopped,
	eWillBeStopped,
	eAllowed
};

#define MAX_PORT_NUM		10
#define MAX_CORE_NUM		(MAX_PORT_NUM*MAX_PORT_NUM)/4
#define MAX_POD_NUM			MAX_PORT_NUM
#define MAX_AGG_PER_POD		MAX_PORT_NUM/2
#define MAX_EDG_PER_POD		MAX_PORT_NUM/2
#define MAX_HOST_PER_EDG	35

#define MAX_FLOW_PER_LINK	MAX_PORT_NUM+1

#define MAX_FLOW_NUM 10001

#define RX		0
#define TX		1
#define DBG		0
#define DBG_MIN_MAX_PRIORITY 0
struct SPath
{
	int AggIndex;
	int CoreIndex;
};

struct SFlowEntry: public SRBTreeNode<int>
{
	int Sorted_Index;
	int pairid;
	int flowid;
	int deadline;
	int size;
	int src_pod;
	int src_edg;
	int src_index;
	int dst_pod;
	int dst_edg;
	int dst_index;

	SFlowEntry(int pairid=0,int fid=0,int size=0,int deadline=0,int src_pod=0,int src_edg=0,int src_index=0,int dst_pod=0,int dst_edg=0,int dst_index=0);
	void Copy(SFlowEntry sEntry);
	bool operator<(const SFlowEntry& other) const;
	bool operator!=(const SFlowEntry& other) const;
	SFlowEntry& operator=(const SFlowEntry& other);
};
struct SSortedTableEntry: public SRBTreeNode<int>
{
	int OrgIndex;//Its Corresponding Index in FlowTable
	int pairid;
	int flowid;
	int deadline;

	EFlowState eState;
	EFlowState ePreState;

	double start_time;
	double last_time;//last time that the flow got the permission.
	int remaining_size;
	int remaining_deadline;

	SPath sPath;

	SSortedTableEntry(int pairid=0,int fid=0,int size=0,int deadline=0, double start_time=0)
	{
		OrgIndex=0;//parent
		this->pairid=pairid;
		this->flowid=fid;
		this->deadline=deadline;
		this->start_time=start_time;
		last_time=0;
		this->remaining_size=size;
		this->remaining_deadline=deadline;
		eState = eInit;
		ePreState = eInit;
		sPath.AggIndex=(-1);
		sPath.CoreIndex=(-1);
	};
	void Copy(SSortedTableEntry sEntry);
	bool operator<(const SSortedTableEntry& other) const;
	bool operator!=(const SSortedTableEntry& other) const;
	SSortedTableEntry& operator=(const SSortedTableEntry& other);

};
struct SFlowTable
{
	//We could make the size a param and bind it too.
	SFlowEntry sTable[MAX_FLOW_NUM];//Max size is 10,000 flows
};
struct SSortedTable
{
	//We could make the size a param and bind it too.
	SSortedTableEntry sTable[MAX_FLOW_NUM];//Max size is 10,000 flows
};

struct SFlow: public SRBTreeNode<int>
{
	SSortedTableEntry *sEntry;
	SFlow(SSortedTableEntry *sEntry=NULL);
	void Copy(SFlow sEntry);
	bool operator<(const SFlow& other) const;
	bool operator!=(const SFlow& other) const;
	SFlow& operator=(const SFlow& other);

	//unique function in list works with the following!
	bool operator==(const SFlow& other) const;
};

struct SFlows
{
	//Fixme: Make it dynamic
	//Max size is 10 flows
	SFlow sTable[MAX_FLOW_PER_LINK];
};

struct SLink
{
	//Fixme: Till Now: we sort flows by their priorities. So to find a flow with its FlowID, we should linearly search it.
	void UpdatePriority(int fid, int priority);
	CRBTree<SFlow> cFlowTree;
	SFlows sFlows;
	int RBW;					//Remaining BW
};


class cController: public TclObject
{
public:
	cController();
	void SetRemainingSize(int pairid,int fid,int size,int deadline,int src_pod,
			int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index,int RemainingSize);
	void NewRequest(int pairid, int fid,int size,int deadline,int src_pod,
			int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index);
	void RemoveRequest(int pairid, int fid,int size,int deadline,int src_pod,
			int src_edg,int src_index,int dst_pod,int dst_edg,int dst_index);
	void Init();
	/**
	 * Scheduling Related Functions
	 */
	void Schedule(int index);
	void ReSchedule(int index);
	bool CheckAvailableBW(int index);
	bool AllocateBW(int nIndex);
	bool FindPath(int index,list<int> &FinalList,SPath &sPath);
	void RemoveFlow(int OrgIndex);
	void AddFlow(int OrgIndex);
	void ManageNewInsertion(int OrgIndex,list<int> FinalList,SPath sPath);
	void ShowLinksFlows(SLink *sLink);

	void SendSignal(int nIndex,EFlowState eState);
//	int SetPath(int *pPath, int nSize=4);
	virtual int command(int argc, const char*const* argv);
	inline double now() { return Scheduler::instance().clock(); }
	void ShowTable();
	void SetTop(int NumHostPerEdge, int NumEdgPerPod, int NumAggPerPod, int NumCorPerAgg,int NumPod, int NumCore);
	void SetBW(int HostEdgeBW, int EdgeAggBW, int AggCoreBW);

private:
	void InitializeBW();
	void Update();
private:
	/**
	 * We wont use dynamic structure here.
	 * We saw issues with triple pointer initialization in NS2.
	 * So here static arrays are used.
	 */

	SLink HostEdgeLink[2][MAX_POD_NUM][MAX_EDG_PER_POD][MAX_HOST_PER_EDG]; // [a][b][c] a:pod index, b: edge per pod index, c: host per edge index
	SLink EdgeAggrLink[2][MAX_POD_NUM][MAX_EDG_PER_POD][MAX_AGG_PER_POD];
	SLink AggrCoreLink[2][MAX_POD_NUM][MAX_AGG_PER_POD][MAX_CORE_NUM];

public:
	CRBTree<SFlowEntry> cFlowTree;
	SFlowTable sFlowTable;

	CRBTree<SSortedTableEntry> cSortedTree;
	SSortedTable sSortedTable;

	int NumCore;
	int NumPod;
	int NumCorPerAgg;
	int NumAggPerPod;
	int NumEdgPerPod;
	int NumHostPerEdge;

	int HostEdgeBW;
	int EdgeAggBW;
	int AggCoreBW;
};

static class ControllerClass : public TclClass {
public:
	ControllerClass() : TclClass("Controller") {}
	TclObject* create(int, const char*const*) {
		return (new cController());
	}
} class_controller_dcn;


#endif /* NS_2_34_COMMON_CONTROLLER_H_ */
