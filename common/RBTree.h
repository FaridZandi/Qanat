/** ====================================================================
 * 		RBTree.h
 *
 *  	Copyright 2015,  Abbasloo, All rights reserved.
 *      Email : ab.@nyu.edu
 *
 * ===================================================================== **/

#ifndef NS_2_34_COMMON_RBTREE_H_
#define NS_2_34_COMMON_RBTREE_H_


#include <stdio.h>
#include "common.h"

enum EColor{Red,Black};
enum EAddType{TreeBase,IndexBase};

#define RBDBG 5
#define RBNULL (-1)
/**
*  Red Black tree node Str.
*/
template<class Type>
struct SDLLPrimary
{
	Type nNext;
	Type nPrev;
}__attribute__((packed));

/**
 * IMPORTANT:"class Type" HAVE TO BE A SIGNED TYPENAME which includes "Signed Char,Signed Short int & Signed int"
 */
template<class Type>
struct SRBTreeNode : protected SDLLPrimary<Type>
{
public:
	template<class xEntry> friend class CRBTree;
protected:
	Type nRight;
	Type nLeft;
	Type nParent;	//nParent for Index 0 Points to the Root of Tree (m_pxTable[0].nParent)!
	Type nColor;	//nColor for Index 0 Shows the Number of Nodes in Tree (Number of Active Nodes).
	Type nFree;		//nFree.0(nFree%2) = 1 ==> Free -	nFree.0 = 0 ==> Full -	(nFree/2) > 0 Indicates Next Free Index.
//	virtual const xEntry& operator = (const xEntry& x) = 0;

}__attribute__((packed));

/**
*   the Red-Black tree encapsulates basic dictionary operator such as find,insert,erase and so on
*   the insert and erase function implemented by iterate method.
*/

template<class xEntry>
class CRBTree{
public:

	CRBTree(xEntry* pxTable, int nEntryCount)
	{
		m_nAddType = TreeBase;
		m_pxTable = pxTable;
		m_nEntryCount = nEntryCount;
	};
	//-------------------------------------------------------------
	CRBTree()__attribute__ ((noinline))
	{
		m_nAddType = TreeBase;
		m_pxTable = 0;
		m_nEntryCount = 0;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	~CRBTree()__attribute__ ((noinline)){}
	//-------------------------------------------------------------
	/**Initialize Table Entries------(nKindOfLL)?TreeBase:IndexBase;
	 *
	 * @param pxTable
	 * @param nEntryCount
	 * @param nReset
	 * @param nKindOfLL
	 * @return
	 */
	int Initialize(xEntry* pxTable, int nEntryCount, int nReset, int nKindOfLL= true) __attribute__ ((noinline))
	{
		m_pxTable = pxTable;
		if(CHECK_POINTER(m_pxTable))
		{
			m_nEntryCount = nEntryCount;
			if (nKindOfLL)
				m_nAddType = TreeBase;
			else
				m_nAddType = IndexBase;

			if(nReset)
			{
				//Initialize all entries
				//Root:
				//Allocator: Reserved in Location "0" of Array
				m_pxTable[0].nParent = RBNULL;
				m_pxTable[0].nLeft = RBNULL;
				m_pxTable[0].nParent = RBNULL;
				m_pxTable[0].nRight = RBNULL;
				m_pxTable[0].nFree = 3;
				m_pxTable[0].nColor = 0;

				m_pxTable[0].nNext = 0;
				m_pxTable[0].nPrev = 0;

				for(int nIndex=1; nIndex<m_nEntryCount;nIndex++)
				{
					m_pxTable[nIndex].nNext = 0;
					m_pxTable[nIndex].nPrev = 0;
					m_pxTable[nIndex].nLeft = RBNULL;
					m_pxTable[nIndex].nParent = RBNULL;
					m_pxTable[nIndex].nRight = RBNULL;
					m_pxTable[nIndex].nColor = Red;
					int n = ((nIndex+1)% m_nEntryCount);
					m_pxTable[nIndex].nFree = (2 * ((n)?n:1)) + 1;
				}
			}
		}
		else
		{
			DBGWARN("Error:Null pointer\n\r");
			m_nEntryCount = 0 ;
			return ERES_ERR_OPERATION_FAILED;
		}
		return ERES_SUCCESS;
	}
	//-------------------------------------------------------------

	int Initialize() __attribute__ ((noinline))
	{
		if(CHECK_POINTER(m_pxTable))
		{
			//Initialize all entries
			//Root:
			//Allocator: Reserved in Location "0" of Array
			m_pxTable[0].nParent = RBNULL;
			m_pxTable[0].nLeft = RBNULL;
			m_pxTable[0].nParent = RBNULL;
			m_pxTable[0].nRight = RBNULL;
			m_pxTable[0].nFree = 3;
			m_pxTable[0].nColor = 0;

			m_pxTable[0].nNext = 0;
			m_pxTable[0].nPrev = 0;

			for(int nIndex=1; nIndex<m_nEntryCount;nIndex++)
			{
				m_pxTable[nIndex].nNext = 0;
				m_pxTable[nIndex].nPrev = 0;
				m_pxTable[nIndex].nLeft = RBNULL;
				m_pxTable[nIndex].nParent = RBNULL;
				m_pxTable[nIndex].nRight = RBNULL;
				m_pxTable[nIndex].nColor = Red;
				int n = ((nIndex+1)% m_nEntryCount);
				m_pxTable[nIndex].nFree = (2 * ((n)?n:1)) + 1;
			}
		}
		else
		{
			DBGWARN("Error:Null pointer\n\r");
			m_nEntryCount = 0 ;
			return ERES_ERR_OPERATION_FAILED;
		}
		return ERES_SUCCESS;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/** the number of nodes in the red black tree
	 *
	 * @return
	 */
	int Size()
	{
		return m_pxTable[0].nColor;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**  whether the tree is empty or not;
	 *
	 * @return
	 */
	bool IsEmpty()
	{
		return m_pxTable[0].nParent==RBNULL;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**get the minimum element ( return location(>0) on success, <0 on error)
	 *
	 * @return
	 */
	const int GetMin()
       {
	      return  GetMin(m_pxTable[0].nParent);
	 }
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**get the minimum element ( return location(>0) on success, <0 on error)
	 *
	 * @return
	 */
	const int GetMax()
	{
		return GetMax(m_pxTable[0].nParent);
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Return Next Index of (exist or not exist)Entry: if any(>0 if exist) else return < 0
	 *
	 * @param x
	 * @return
	 */
	int FindNext(const xEntry &x) __attribute__ ((noinline))
	{
		int tmp=m_pxTable[0].nParent;
		bool bFirstTimeInit = true;
		int nResultIndexTmp = RBNULL;
		int nResultIndex = RBNULL;
		if (0>tmp)
			return RBNULL;
		else
		{
			while (tmp>=0)
			{
				int nRes = Compare(x,m_pxTable[tmp]);
				if (nRes<0)
				{
					tmp=m_pxTable[tmp].nRight;
				}
				else if(nRes>0)
				{
					nResultIndexTmp = tmp ;
					if(!bFirstTimeInit)
					{
						int nRes2 = Compare(m_pxTable[nResultIndexTmp],m_pxTable[nResultIndex]);
						if(nRes2 > 0 )
						{
							nResultIndex = nResultIndexTmp;
						}
					}
					else
					{
						nResultIndex = tmp ;
						bFirstTimeInit = false;
					}

					tmp=m_pxTable[tmp].nLeft;
				}
				else
				{
					return Next(tmp);

					/*tmp=m_pxTable[tmp].nRight;

					if(tmp>=0)
					{
						return (GetMin(tmp));
					}
					else
					{
						return nResultIndex;
					}*/
				}
			}
		}
		return nResultIndex;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Return Previous Index of (exist or not exist)Entry: if any(>0 if exist) else return < 0
	 *
	 * @param x
	 * @return
	 */
	int FindPrev(const xEntry &x) __attribute__ ((noinline))
	{
		int tmp=m_pxTable[0].nParent;
		bool bFirstTimeInit = true;
		int nResultIndexTmp = RBNULL;
		int nResultIndex = RBNULL;
		if (0>tmp)
			return RBNULL;
		else
		{
			while (tmp>=0)
			{
				int nRes = Compare(x,m_pxTable[tmp]);
				if (nRes<0)
				{
					nResultIndexTmp = tmp ;
					if(!bFirstTimeInit)
					{
						int nRes2 = Compare(m_pxTable[nResultIndexTmp],m_pxTable[nResultIndex]);
						if(nRes2 < 0 )
						{
							nResultIndex = nResultIndexTmp;
						}
					}
					else
					{
						nResultIndex = tmp ;
						bFirstTimeInit = false;
					}

					tmp=m_pxTable[tmp].nRight;
				}
				else if(nRes>0)
				{
					tmp=m_pxTable[tmp].nLeft;
				}
				else
				{
					tmp=m_pxTable[tmp].nLeft;
					if(tmp>=0)
					{
						return (GetMax(tmp));
					}
					else
					{
						return nResultIndex;
					}
				}
			}
		}
		return nResultIndex;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Return Next Index of (exist)Entry if any(>0 if exist)
	 *
	 * @param rxEntry
	 * @param nArrayIndex
	 * @return
	 */
	int Next(xEntry& rxEntry)
	{
		int nIndex = Find(rxEntry);
		if(nIndex>0)
		{
			if(IsIndexActive(nIndex))
				return m_pxTable[nIndex].nNext;
			else
				return ERES_ERR_OPERATION_FAILED;
		}
		else
			return ERES_ERR_OPERATION_FAILED;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Return Next Index of (exist)Entry if any(>0 if exist)
	 *
	 * @param rxEntry
	 * @param nArrayIndex
	 * @return
	 */
	//FIXME: {SA} next failed if operator= has not been implemented by user(it may affect other functions too)
	int Next(int nArrayIndex)
	{
		if((nArrayIndex<=0)|| (nArrayIndex>=m_nEntryCount))//not Found
		{
			DBGPRINT(RBDBG,1,"Out of Range index {%d}!\n\r",nArrayIndex);
			return ERES_ERR_OPERATION_FAILED;
		}
		if(IsIndexActive(nArrayIndex))
			return m_pxTable[nArrayIndex].nNext;

		DBGPRINT(RBDBG,1,"There is no such an index {%d}, This Index is Free!\n\r",nArrayIndex);
		return RBNULL;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**return Index if Entry found(>0)
	 *
	 * @param x
	 * @param nArrayIndex
	 * @return
	 */
	int Find(const xEntry &x) __attribute__ ((noinline))
	{
		int nIndex = Find(m_pxTable[0].nParent,x);
		if((nIndex>0))
		{
			if(IsIndexActive(nIndex))
				return nIndex;
			else
			{
				DBGPRINT(RBDBG,1,"The Entry is Free!\n\r");
				return ERES_ERR_OPERATION_FAILED;
			}
		}
		else
		{
			DBGPRINT(RBDBG,1,"No Entry Found!\n\r");
			return ERES_ERR_OPERATION_FAILED;
		}
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**return Pointer to Entry if Entry found, else NULL
	 *
	 * @param x
	 * @param nArrayIndex
	 * @return
	 */
	xEntry* FindE(const xEntry &x)
	{
		int nIndex = Find(m_pxTable[0].nParent,x);
		if(nIndex>0)
		{
			if(IsIndexActive(nIndex))
				return &m_pxTable[nIndex];
			else
			{
				DBGPRINT(RBDBG,1,"There is no such an Instance out there!\n\r");
				return NULL;
			}

		}
		else
			return NULL;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**insert element with unique Key.if find the exist element,give an Error!
	 *
	 * @param x
	 * @param nArrayIndex
	 * @return > 0 on success
	 */
	int Add(const xEntry &x,int nArrayIndex=0) __attribute__ ((noinline))
	{
		int nFinalIndex;
		if(CHECK_POINTER(m_pxTable))
		{
			//first find the insert position (node's Location in Tree) and its nParent ,call InserAux
			int y=m_pxTable[0].nParent;
			int p=m_pxTable[0].nParent;
			int nNextResultIndex = RBNULL;
			bool bNextFirstTimeInit = true;
			int nNextResultIndexTmp = RBNULL;
			bool bPrevFirstTimeInit = true;
			int nPrevResultIndexTmp = RBNULL;
			int nPrevResultIndex = RBNULL;
			while (y>=0)
			{
				p=y;
				int nRes = Compare(x,m_pxTable[y]);//1:x<y -1:x>y 0: x=y
				if (nRes > 0)
				{
					nNextResultIndexTmp = y ;
					if(!bNextFirstTimeInit)
					{
						int nRes2 = Compare(m_pxTable[nNextResultIndexTmp],m_pxTable[nNextResultIndex]);
						if(nRes2 > 0 )
						{
							nNextResultIndex = nNextResultIndexTmp;
						}
					}
					else
					{
						nNextResultIndex = y ;
						bNextFirstTimeInit = false;
					}


					y = m_pxTable[y].nLeft;
				}
				else if(nRes < 0)
				{
					nPrevResultIndexTmp = y ;
					if(!bPrevFirstTimeInit)
					{
						int nRes2 = Compare(m_pxTable[nPrevResultIndexTmp],m_pxTable[nPrevResultIndex]);
						if(nRes2 < 0 )
						{
							nPrevResultIndex = nPrevResultIndexTmp;
						}
					}
					else
					{
						nPrevResultIndex = y ;
						bPrevFirstTimeInit = false;
					}

					y = m_pxTable[y].nRight;
				}
				else
				{
					DBGPRINT(RBDBG,1,"Error:Equal Insertion\n\r");
					return ERES_ERR_OPERATION_FAILED;
				}
			}

			if(InserAux(p,x,nArrayIndex,nNextResultIndex,nPrevResultIndex,nFinalIndex)<0)
			{
				DBGPRINT(RBDBG,1,"Error: \n\r");
				return ERES_ERR_OPERATION_FAILED;
			}
			else
			{
				return nFinalIndex;
			}
		}
		else
		{
			DBGWARN("Error:Null pointer\n\r");
			return ERES_ERR_OPERATION_FAILED;
		}
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Modify Existing Entry of table( return location(>0) on success, <0 on error)
	 *
	 * @param rxEntry
	 * @return
	 */
	int Modify(xEntry& rxEntry)
	{
		if(CHECK_POINTER(m_pxTable))
		{
			int nIndex;
			nIndex = Find(rxEntry);
			if(nIndex>0)
				m_pxTable[nIndex] = rxEntry;
			else
				return ERES_ERR_OPERATION_FAILED;

			return nIndex;
		}
		else
		{
			DBGWARN("Error:Null pointer\n\r");
			return ERES_ERR_OPERATION_FAILED;
		}
	}
    //-------------------------------------------------------------
	//-------------------------------------------------------------
	/**Modify Existing Entry of table (with index = nArrayIndex)( return location(>0) on success, <0 on error)
	 *
	 * @param rxEntry
	 * @param nArrayIndex
	 * @return
	 */
	int Modify(int nArrayIndex,xEntry& rxEntry)
	{
		if (nArrayIndex!=0)
		{
			if ((nArrayIndex > 0) && (nArrayIndex < m_nEntryCount))
			{
				m_pxTable[nArrayIndex] = rxEntry;
				return nArrayIndex;
			}
			else
				return ERES_ERR_OPERATION_FAILED;
		}
		else
		{
			return ERES_ERR_OPERATION_FAILED;
		}
	}
    //-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Erase the first element with key that equals to x
	 *
	 * @param x
	 * @return
	 */
	int Del(const xEntry&x) __attribute__ ((noinline))
	{
		//only erase the first x
		int y =m_pxTable[0].nParent;
		while (y>=0)//Root should not Point to 0 index
		{
			int nRes = Compare(x,m_pxTable[y]);
			if (nRes>0)
				y = m_pxTable[y].nLeft;
			else if (nRes<0)
				y = m_pxTable[y].nRight;
			else break;
		}
		return EraseAux(y);
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**erase the element in indexe nIndex
	 *
	 * @param nIndex
	 * @return
	 */
	int Del(int nIndex) __attribute__ ((noinline))
	{
		return EraseAux(nIndex);
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**First(>0 if any)
	 *
	 * @return
	 */
	int First(void)
	{
		return ((IsEmpty())?RBNULL:m_pxTable[0].nNext);
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	int Last()
	{
		return ((IsEmpty())?RBNULL:m_pxTable[0].nPrev);
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**true if index is not free
	 *
	 * @param nIndex
	 * @return
	 */
	int IsIndexActive(int nIndex)
	{
		if(!CHECK_VAR_RANGE_NB(nIndex, 0, m_nEntryCount))
		{
			DBGPRINT(RBDBG,1,"Invalid Index(%d)\n", nIndex);
			return false;
		}

		return (m_pxTable[nIndex].nFree%2)?false:true;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	/**Return Pointer To Entry
	 *
	 * @param nIndex
	 * @return
	 */
	xEntry* Entry(int nIndex) __attribute__ ((noinline))
	{
		if(!CHECK_VAR_RANGE_NB(nIndex, 0, m_nEntryCount))
		{
			DBGERROR("Invalid Index(%d)\n", nIndex);
			return NULL;
		}
		if(CHECK_POINTER(m_pxTable))
		{
			return &m_pxTable[nIndex];
		}
		else
		{
			DBGWARN("Error:Null pointer\n\r");
			return NULL;
		}
	}
	//-------------------------------------------------------------

private:
	int m_nAddType;
	xEntry *m_pxTable;
	int m_nEntryCount;
private:
	//-------------------------------------------------------------
	//return Index > 0 if allocation successes else return Index < 0
	int Allocator(int nChosenIndex) __attribute__ ((noinline))
	{
		if(m_nAddType==TreeBase)
		{
			if(nChosenIndex==0)
			{
				int nNextFreeIndex = m_pxTable[0].nFree/2;
				int nFree = m_pxTable[nNextFreeIndex].nFree%2;
				if(nNextFreeIndex==0)
				{
					DBGPRINT(RBDBG,1,"sorry Bro!The array is full\n\r");
					return ERES_ERR_OPERATION_FAILED;
				}
				else if(nNextFreeIndex< 0)
				{
					DBGPRINT(RBDBG,1,"Next is Null.\n\r");
					return ERES_ERR_OPERATION_FAILED;
				}
				else
				{
					if(nFree)
					{
//						DBGPRINT(RBDBG,1,"nIndex = %d\n\r",nNextFreeIndex);
						return (nNextFreeIndex);
					}
					else
					{
						DBGPRINT(RBDBG,1,"Invalid Allocation: Index = %d\n\r",nNextFreeIndex);
						return ERES_ERR_INVALID_OPERATION;
					}
				}

			}
			else
			{
				DBGPRINT(RBDBG,1,"The Type of addition have been set to Tree Base while an Index has been entered, Are You Kidding me?! \n\r");
				return ERES_ERR_OPERATION_FAILED;
			}
		}
		else
		{
			if(nChosenIndex!=0)
			{
				int nNextFreeIndex = nChosenIndex;
				int nFree = m_pxTable[nNextFreeIndex].nFree%2;
				if(nNextFreeIndex==0)
				{
					DBGPRINT(RBDBG,1,"sorry Bro!Invalid Location, The index 0 is reserved !\n\r");
					return ERES_ERR_OPERATION_FAILED;
				}
				else if(nNextFreeIndex< 0)
				{
					DBGPRINT(RBDBG,1,"Next Is Null.\n\r");
					return ERES_ERR_OPERATION_FAILED;
				}
				else
				{
					if(nFree)
					{
//						DBGPRINT(RBDBG,1,"nArrayIndex = %d\n\r",nNextFreeIndex);
						return (nNextFreeIndex);
					}
					else
					{
						DBGPRINT(RBDBG,1,"sorry Bro! The Node is not Free!{%d}\n\r",nNextFreeIndex);
						return ERES_ERR_INVALID_OPERATION;
					}
				}
			}
			else
			{
				DBGPRINT(RBDBG,1,"The Type of addition have been set to Index Base while a Valid Index has not been entered, Are You Kidding me?! \n\r");
				return ERES_ERR_OPERATION_FAILED;
			}
		}
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
    int GetMin(int begin) __attribute__ ((noinline))
    {
		if (0>begin)
		{
			DBGPRINT(RBDBG,1,"the tree is empty,get min error!");
			return ERES_ERR_OPERATION_FAILED;
		}
		else
		{
			int min=begin;
			while (m_pxTable[min].nLeft>=0)
				min=m_pxTable[min].nLeft;
			return min;
		}
    }
	//-------------------------------------------------------------

	//-------------------------------------------------------------
    int GetMax(int begin)
    {
		if (0>begin)
		{
			DBGPRINT(RBDBG,1,"the tree is empty,get max error!");
			return ERES_ERR_OPERATION_FAILED;
		}
		else
		{
			int max=begin;
			while (m_pxTable[max].nRight>=0)
			{
				max=m_pxTable[max].nRight;
			}
			return max;
		}
    }
	//-------------------------------------------------------------

	//-------------------------------------------------------------
    int Find(int begin,const xEntry &x) __attribute__ ((noinline))
	{
		if (0>begin)
			return RBNULL;
		else
		{
			int tmp=begin;
			while (tmp>=0)
			{
				int nRes = Compare(x,m_pxTable[tmp]);
				if (nRes<0)
					tmp=m_pxTable[tmp].nRight;
				else if(nRes>0)
					tmp=m_pxTable[tmp].nLeft;
				else break;
			}
			return tmp;
		}
	}
	//-------------------------------------------------------------

    //-------------------------------------------------------------
	void RotateLeft(int t) __attribute__ ((noinline))
	{
		int tmp = m_pxTable[t].nRight;
		m_pxTable[t].nRight =  m_pxTable[tmp].nLeft;

		if (m_pxTable[tmp].nLeft>=0)
			m_pxTable[m_pxTable[tmp].nLeft].nParent = t;

		m_pxTable[tmp].nLeft =t;
		m_pxTable[tmp].nParent = m_pxTable[t].nParent;

		if (t==m_pxTable[0].nParent)
			m_pxTable[0].nParent = tmp;
		else if (t==m_pxTable[m_pxTable[t].nParent].nLeft)
			m_pxTable[m_pxTable[t].nParent].nLeft = tmp;
		else
			m_pxTable[m_pxTable[t].nParent].nRight=tmp;

		m_pxTable[t].nParent =tmp;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	void RotateRight(int t) __attribute__ ((noinline))
	{

		int tmp=m_pxTable[t].nLeft;
		m_pxTable[t].nLeft = m_pxTable[tmp].nRight;

		if (m_pxTable[tmp].nRight>=0)
			m_pxTable[m_pxTable[tmp].nRight].nParent = t;

		m_pxTable[tmp].nRight = t;


//		if (m_pxTable[t].nParent>=0)
		m_pxTable[tmp].nParent = m_pxTable[t].nParent;
//		DBGPRINT(RBDBG,1,"tmp= %d - t = %d - m_pxTable[tmp].nParent = %d \n\r",tmp,t,m_pxTable[tmp].nParent );

		if (t==m_pxTable[0].nParent)
		{
			m_pxTable[0].nParent=tmp;
		}
		else if (t==m_pxTable[m_pxTable[t].nParent].nLeft)
		{
			m_pxTable[m_pxTable[t].nParent].nLeft = tmp;
		}
		else
		{
			m_pxTable[m_pxTable[t].nParent].nRight = tmp;
		}

//	       DBGPRINT(RBDBG,1,"0) P:%d - R:%d -L:%d -  \n\r",m_pxTable[0].nParent,m_pxTable[0].nRight,m_pxTable[0].nLeft);

		m_pxTable[t].nParent = tmp;
//	       DBGPRINT(RBDBG,1,"0) P:%d - R:%d -L:%d -  \n\r",m_pxTable[0].nParent,m_pxTable[0].nRight,m_pxTable[0].nLeft);

//		DBGPRINT(RBDBG,1,"Grand Parent:%d - Parent:%d |||||||||||||| Grand_Parent:%d -  \n\r",t,tmp,m_pxTable[t].nParent);

	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	int EraseAux(int z) __attribute__ ((noinline))
	{
		if (0>=z)
		{
			DBGPRINT(RBDBG,1,"Nothing Found Baby!\n\r");
			return ERES_ERR_OPERATION_FAILED;
		}
		//Free the z Location:


		int x=0;
		int x_parent=0;
		int y = z;
		if(m_pxTable[y].nLeft==RBNULL)
			x = m_pxTable[y].nRight;
		else if (m_pxTable[y].nRight==RBNULL)
			x = m_pxTable[y].nLeft;
		else
		{
			y=m_pxTable[y].nRight;
			while (m_pxTable[y].nLeft>=0)
				y = m_pxTable[y].nLeft;
			x= m_pxTable[y].nRight;
		}
//		DBGPRINT(RBDBG,5," y = %d, x = %d , z = %d\n\r",y,x,z);
		if (y!=z)
		{
			int nTmp;
			//y is higher level replace node;
			//x is nRight-leftmost (lower level ) replace node;
			m_pxTable[m_pxTable[z].nLeft].nParent = y;
			m_pxTable[y].nLeft=m_pxTable[z].nLeft;

//			DBGPRINT(RBDBG,1," m_pxTable[z].nLeft = %d, m_pxTable[m_pxTable[z].nLeft].nParent = %d , m_pxTable[y].nLeft = %d\n\r",
//								m_pxTable[z].nLeft,m_pxTable[m_pxTable[z].nLeft].nParent,m_pxTable[y].nLeft);
			if (y!=m_pxTable[z].nRight)
			{
				x_parent = m_pxTable[y].nParent;
				if (x>=0)
					m_pxTable[x].nParent = m_pxTable[y].nParent;
				m_pxTable[m_pxTable[y].nParent].nLeft = x;
				m_pxTable[y].nRight=m_pxTable[z].nRight;
				m_pxTable[m_pxTable[z].nRight].nParent = y ;
//				DBGPRINT(RBDBG,1,"%d) = %d---p = %d,l= %d R = %d Color= %d ,Free = %d ,NFree = %d \n\r\n\r",y,m_pxTable[y].a,m_pxTable[y].nParent,m_pxTable[y].nLeft,m_pxTable[y].nRight,m_pxTable[y].nColor,(m_pxTable[y].nFree%2),(m_pxTable[y].nFree/2));

			}
			else
				x_parent = y;
			if (m_pxTable[0].nParent == z)
				m_pxTable[0].nParent = y;
			else if(z==m_pxTable[m_pxTable[z].nParent].nLeft)
				m_pxTable[m_pxTable[z].nParent].nLeft = y;
			else
				m_pxTable[m_pxTable[z].nParent].nRight = y;
			m_pxTable[y].nParent=m_pxTable[z].nParent;
//			DBGPRINT(RBDBG,1,"m_pxTable[y].nColor = %d , m_pxTable[z].nColor = %d\n\r",m_pxTable[y].nColor,m_pxTable[z].nColor);

//			Swap(m_pxTable[y].nColor,m_pxTable[z].nColor);
			nTmp = m_pxTable[y].nColor;
			m_pxTable[y].nColor = m_pxTable[z].nColor;
			m_pxTable[z].nColor = nTmp;
//			DBGPRINT(RBDBG,1,"m_pxTable[y].nColor = %d , m_pxTable[z].nColor = %d\n\r",m_pxTable[y].nColor,m_pxTable[z].nColor);
//			DBGPRINT(RBDBG,1,"m_pxTable[m_pxTable[z].nParent].nRight = %d ,m_pxTable[y].nParent = %d \n\r",m_pxTable[m_pxTable[z].nParent].nRight,m_pxTable[y].nParent);
			y = z;
//			DBGPRINT(RBDBG,1," y = %d, x = %d , z = %d Z.Color = %d\n\r",y,x,z,m_pxTable[y].nColor);
		}
		else
		{
//			DBGPRINT(RBDBG,1," I'm Here !-------------else (y!=z) \n\r");
			//y==z is delete node;
			x_parent = m_pxTable[y].nParent;
			if (x>=0)
				m_pxTable[x].nParent = m_pxTable[y].nParent;
			if (z==m_pxTable[0].nParent)
				m_pxTable[0].nParent = x;
			else
			{
				if (z==m_pxTable[m_pxTable[z].nParent].nLeft)
					m_pxTable[m_pxTable[z].nParent].nLeft = x;
				else
					m_pxTable[m_pxTable[z].nParent].nRight = x;
			}
		}
//		DBGPRINT(RBDBG,1,"m_pxTable[y].nColor = %d , m_pxTable[z].nColor = %d Red = %d\n\r",m_pxTable[y].nColor,m_pxTable[z].nColor,Red);
		if (m_pxTable[y].nColor!=Red)
		{
//			DBGPRINT(RBDBG,1," I'm Here !-------------m_pxTable[y].nColor!=Red \n\r");
			EraseFixup(x,x_parent);
		}
		ResetNode(z);
		m_pxTable[z].nFree = m_pxTable[0].nFree;
		m_pxTable[z].nFree |= 0x1;
		m_pxTable[0].nFree = ( z << 1 ) + 1;
		//==========================================
		if(m_pxTable[z].nNext>0)
		{
			if(m_pxTable[z].nPrev>0)
			{
				m_pxTable[m_pxTable[z].nPrev].nNext = m_pxTable[z].nNext;
				m_pxTable[m_pxTable[z].nNext].nPrev = m_pxTable[z].nPrev;
			}
			else
			{
				m_pxTable[m_pxTable[z].nNext].nPrev = 0;
				m_pxTable[0].nNext = m_pxTable[z].nNext;
			}
		}
		else
		{
			if(m_pxTable[z].nPrev>0)
			{
				m_pxTable[m_pxTable[z].nPrev].nNext = 0;
				m_pxTable[0].nPrev = m_pxTable[z].nPrev;
			}
			else
			{
				m_pxTable[0].nNext = 0;
				m_pxTable[0].nPrev = 0;
			}
		}
		//==========================================
		--m_pxTable[0].nColor;

/*
		for(int i=0;i<20;i++)
			DBGPRINT(RBDBG,1,"%d) = %d---p = %d,l= %d R = %d Color= %d ,Free = %d ,NFree = %d , nNext = %d, nPrev = %d \n\r",i,m_pxTable[i].a,m_pxTable[i].nParent,m_pxTable[i].nLeft,m_pxTable[i].nRight,m_pxTable[i].nColor,(m_pxTable[i].nFree%2),(m_pxTable[i].nFree/2), m_pxTable[i].nNext,m_pxTable[i].nPrev);
*/

/*
		for(int i=0;i<30;i++)
				DBGPRINT(RBDBG,1,"%d) = %d---p = %d,l= %d R = %d Color= %d ,Free = %d ,NFree = %d \n\r\n\r",i,m_pxTable[i].a,m_pxTable[i].nParent,m_pxTable[i].nLeft,m_pxTable[i].nRight,m_pxTable[i].nColor,(m_pxTable[i].nFree%2),(m_pxTable[i].nFree/2));
*/

		return ERES_SUCCESS;
//		delete y;

	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//update the x and its nParent link,then InserFixup

	int InserAux(int p,const xEntry&x,int nChosenIndex,int nNextResultIndex,int nPrevResultIndex,int& nFinalIndex) __attribute__ ((noinline))
	{
//		Node*cur=new Node(x);

		int nIndex = Allocator(nChosenIndex);

		if(nIndex>0)
		{

			if (0>p)
			{
	//			m_pxTable[m_pxTable[0].nParent] = x;//??
				m_pxTable[0].nParent = nIndex;
			}
			else
			{
				int nRes = Compare(x,m_pxTable[p]);
				if (nRes>0)
				{
	//				x.nParent = p;
	//				m_pxTable[m_pxTable[p].nLeft] = x;//??
					m_pxTable[p].nLeft = nIndex;
					m_pxTable[m_pxTable[p].nLeft].nParent = p;
//					nIndex = m_pxTable[p].nLeft ;
				}
				else if (nRes<0)
				{
	//				m_pxTable[m_pxTable[p].nRight] = x;//??
	//				p->nRight=cur;
					m_pxTable[p].nRight = nIndex;
					m_pxTable[m_pxTable[p].nRight].nParent = p;
	//				cur->nParent=p;
//					nIndex = m_pxTable[p].nRight;
				}
				else
				{
					DBGPRINT(RBDBG,1,"Error: Equality!\n\r");
				}
			}
			InserFixup(nIndex);
			//==========================================
			if(nNextResultIndex>0)
			{
				m_pxTable[nIndex].nNext = nNextResultIndex;
				if(m_pxTable[nNextResultIndex].nPrev>0)
					m_pxTable[m_pxTable[nNextResultIndex].nPrev].nNext = nIndex;
				m_pxTable[nNextResultIndex].nPrev = nIndex;
			}
			else
			{
				m_pxTable[nIndex].nNext = 0;
			}
			if(nPrevResultIndex>0)
			{
				m_pxTable[nIndex].nPrev = nPrevResultIndex;
				m_pxTable[m_pxTable[nIndex].nPrev].nNext = nIndex;
			}
			else
			{
				m_pxTable[nIndex].nPrev = 0;
			}

			if(m_pxTable[nIndex].nPrev == 0)
				m_pxTable[0].nNext = nIndex;

			if(m_pxTable[nIndex].nNext == 0)
				m_pxTable[0].nPrev = nIndex;
			//==========================================
			m_pxTable[nIndex] = x;
			//==========================================
			m_pxTable[nIndex].nFree &= 0xFFFFFFFE;
			if(!IsIndexActive((m_pxTable[nIndex].nFree/2)))
				m_pxTable[0].nFree = ( (m_pxTable[nIndex].nFree/2) << 1 ) + 1;
			else
				m_pxTable[0].nFree = 0x1;
			++m_pxTable[0].nColor;
			//==========================================

/*
			DBGPRINT(RBDBG,1,"************** Root of Tree ************\n\r");
			DBGPRINT(RBDBG,1,"0) = ---p = %d,l= %d R = %d Color= %d ,Free = %d ,NFree = %d nNext = %d, nPrev = %d \n\r",m_pxTable[0].nParent,m_pxTable[0].nLeft,m_pxTable[0].nRight,m_pxTable[0].nColor,(m_pxTable[0].nFree%2),(m_pxTable[0].nFree/2), m_pxTable[0].nNext,m_pxTable[0].nPrev);
*/

/*

			DBGPRINT(RBDBG,1,"============================================================================================\n\r");
			for(int i=0;i<20;i++)
				DBGPRINT(RBDBG,1,"%d) = %d---p = %d,l= %d R = %d Color= %d ,Free = %d ,NFree = %d , nNext = %d, nPrev = %d \n\r",i,m_pxTable[i].a,m_pxTable[i].nParent,m_pxTable[i].nLeft,m_pxTable[i].nRight,m_pxTable[i].nColor,(m_pxTable[i].nFree%2),(m_pxTable[i].nFree/2), m_pxTable[i].nNext,m_pxTable[i].nPrev);
*/


		}
		else
		{
			DBGPRINT(RBDBG,1,"Error: Allocation failed\n\r");
			return ERES_ERR_OPERATION_FAILED;
		}
		nFinalIndex = nIndex;
		return ERES_SUCCESS;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	void InserFixup(int nIndex) __attribute__ ((noinline))
	{
		m_pxTable[nIndex].nColor = Red;
		while ((nIndex!=m_pxTable[0].nParent)&&(m_pxTable[m_pxTable[nIndex].nParent].nColor==Red))
		{
			if (m_pxTable[nIndex].nParent==m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nLeft)
			{
//				Node*u=c->nParent->nParent->nRight;
				int nUncle = m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nRight;
				if ((0>nUncle)||(m_pxTable[nUncle].nColor==Black))//Case 5
				{
					if (nIndex==m_pxTable[m_pxTable[nIndex].nParent].nRight)
					{
						nIndex=m_pxTable[nIndex].nParent;
						RotateLeft(nIndex);
					}
					m_pxTable[m_pxTable[nIndex].nParent].nColor=Black;
					m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nColor=Red;
//					DBGPRINT(RBDBG,1,"I am Here - 0 -	me = %d, MyParent = %d MygrandPa = %d\n\r",nIndex,m_pxTable[nIndex].nParent,m_pxTable[m_pxTable[nIndex].nParent].nParent);
					int nGP = m_pxTable[m_pxTable[nIndex].nParent].nParent;
					RotateRight(nGP);
				}
				else
				{
					m_pxTable[m_pxTable[nIndex].nParent].nColor=Black;
					m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nColor=Red;
					m_pxTable[nUncle].nColor = Black;
					nIndex = m_pxTable[m_pxTable[nIndex].nParent].nParent;
				}
			}
			else
			{
				int nUncle = m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nLeft;
				if ((0>nUncle)||(m_pxTable[nUncle].nColor==Black))
				{
					if (nIndex==m_pxTable[m_pxTable[nIndex].nParent].nLeft)
					{
						nIndex=m_pxTable[nIndex].nParent;
//						DBGPRINT(RBDBG,1,"I am Here - 1 -\n\r");
						RotateRight(nIndex);
					}
					m_pxTable[m_pxTable[nIndex].nParent].nColor=Black;
					m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nColor=Red;
					RotateLeft(m_pxTable[m_pxTable[nIndex].nParent].nParent);
				}
				else
				{
					m_pxTable[m_pxTable[nIndex].nParent].nColor=Black;
					m_pxTable[m_pxTable[m_pxTable[nIndex].nParent].nParent].nColor=Red;
					m_pxTable[nUncle].nColor = Black;
					nIndex = m_pxTable[m_pxTable[nIndex].nParent].nParent;
				}
			}
//			nLocalCounter++;
//			DBGPRINT(RBDBG,1,"nLocalCounter = %d - nIndex = %d\n\r",nLocalCounter,nIndex);
		}

		m_pxTable[m_pxTable[0].nParent].nColor=Black;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	void EraseFixup(int c,int p) __attribute__ ((noinline))
	{
		//c is replace node,p is c's nParent
		while ((c!=m_pxTable[0].nParent)&&(c == RBNULL||m_pxTable[c].nColor==Black))
		{
			if (c==m_pxTable[p].nLeft)
			{
				int w=m_pxTable[p].nRight;
				if (m_pxTable[w].nColor==Red)
				{
					m_pxTable[w].nColor=Black;
					m_pxTable[p].nColor=Red;
					RotateLeft(p);
					w = m_pxTable[p].nRight;
				}
				if ((m_pxTable[w].nLeft==RBNULL||m_pxTable[m_pxTable[w].nLeft].nColor==Black)&&
						(m_pxTable[w].nRight==RBNULL||m_pxTable[m_pxTable[w].nRight].nColor==Black))
				{
					m_pxTable[w].nColor=Red;
					c = p;
					p = m_pxTable[p].nParent;
				}
				else
				{
					if ((m_pxTable[w].nRight==RBNULL)||(m_pxTable[m_pxTable[w].nRight].nColor==Black))
					{
						if (m_pxTable[w].nLeft>=0)
							m_pxTable[m_pxTable[w].nLeft].nColor=Black;
						m_pxTable[w].nColor=Red;
						RotateRight(w);
						w = m_pxTable[p].nRight;
					}
					m_pxTable[w].nColor=m_pxTable[p].nColor;
					m_pxTable[p].nColor=Black;
					if (m_pxTable[w].nRight>=0)
						m_pxTable[m_pxTable[w].nRight].nColor=Black;
					RotateLeft(p);
					break;
				}
			}
			else
			{
				int w = m_pxTable[p].nLeft;
				if (m_pxTable[w].nColor==Red)
				{
					m_pxTable[w].nColor=Black;
					m_pxTable[p].nColor=Red;
					RotateRight(p);
					w = m_pxTable[p].nLeft;
				}
				if ((m_pxTable[w].nRight==RBNULL||m_pxTable[m_pxTable[w].nRight].nColor==Black)&&
						(m_pxTable[w].nLeft==RBNULL||m_pxTable[m_pxTable[w].nLeft].nColor==Black))
				{
					m_pxTable[w].nColor=Red;
					c = p;
					p = m_pxTable[p].nParent;
				}
				else
				{
					if (m_pxTable[w].nLeft==RBNULL||m_pxTable[m_pxTable[w].nLeft].nColor==Black)
					{
						if (m_pxTable[w].nRight>=0)
							m_pxTable[m_pxTable[w].nRight].nColor=Black;
						m_pxTable[w].nColor=Red;
						RotateLeft(w);
						w=m_pxTable[p].nLeft;
					}
					m_pxTable[w].nColor = m_pxTable[p].nColor;
					m_pxTable[p].nColor=Black;
					if (m_pxTable[w].nLeft>=0)
						m_pxTable[m_pxTable[w].nLeft].nColor=Black;
					RotateRight(p);
					break;
				}
			}
		}
		if (c>=0)
			m_pxTable[c].nColor=Black;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	void ResetNode(int nIndex)
	{
		m_pxTable[nIndex].nLeft = RBNULL;
		m_pxTable[nIndex].nParent = RBNULL;
		m_pxTable[nIndex].nRight = RBNULL;
		m_pxTable[nIndex].nColor = Red;
//		m_pxTable[nIndex].nFree = 2 * (nIndex+1) + 1;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	int AssignValue(xEntry& xRef, xEntry& xKey)
	{
	     xKey.nParent = xRef.nParent;
	     xKey.nLeft = xRef.nLeft;
	     xKey.nRight = xRef.nRight;
	     xKey.eNodeColor = xRef.eNodeColor;
	     xKey.nNext =  xRef.nNext;
	     xKey.nPrev =  xRef.nPrev;
	     xKey.nFree =  xRef.nFree;

	     return ERES_SUCCESS;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	int Compare(const xEntry& xKeyone, const xEntry& xKeytwo)
	{
	    if (xKeyone < xKeytwo) return 1;
	    else if (xKeytwo != xKeyone) return -1;
	    else return 0;
	}
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	void Swap(int &a,int &b)
	{
		int tmp=a;
		a=b;
		b=tmp;
	}
	//-------------------------------------------------------------

};


#endif /* NS_2_34_COMMON_RBTREE_H_ */
