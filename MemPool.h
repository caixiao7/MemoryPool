#ifndef MEMORY_POOL_
#define MEMORY_POOL_
/* Author: Shawn.C
*  Date: 2015-03-01
*  History:
*  2015-03-02����ɳ��壬ͨ���򵥲��ԣ�ֻ֧�ֵ��߳�
*  2015-06-10: upload to github
*/

//�����Ҫ֧�ֶ��̣߳�����SUPPORT_MULTI_THREAD
#define SUPPORT_MULTI_THREAD

#ifdef SUPPORT_MULTI_THREAD 
#include <process.h>
#endif
#include <stdio.h>
#include <windows.h>

#define DE_BLOCKNUM  25  //Block��Ŀ�����Գ�������
#define DE_NOTENUM   25  //Note��Ŀ�����Գ�������
#define MAX_EXTERN	 25  //������չ��Block��Ŀ����չBlock���ڴ浥Ԫ���ڳ�ʼ���ڴ浥Ԫ��
#define MAX_MEMORYSIZE 	2*1024*1024//2M

//���������־�����Զ���Ϊд���ļ���
void WriteToLog(unsigned char *logInfo);


class MemBlock;//�ڴ������Ե���������ʽ����Note��

class MemNote;//�ڴ浥Ԫ��ӵ��ָ�룬ָ���Լ����ڴ浥Ԫ���û�ͨ��GetBuffer�õ�ָ�롣

class MemPool//�ڴ��
{
public:
	MemPool();
	virtual ~MemPool();
public:
	bool InitPool(int blockNum = DE_BLOCKNUM, int noteNum = DE_NOTENUM, long unitSize = MAX_MEMORYSIZE);
	void DestoryPool();
	MemNote *GetMemoryNote(); 
	MemNote *GetMemoryNote(long memorySize); 
	void ReleaseMemory(MemNote *pMemoryNote);
	long double GetTotalMemorySize();
	long double GetFreeMemorySize();
private:
	MemBlock *blockHead_;
	long double memorySize_;
	long double freeSize_;
	long unitSize_;
	unsigned char *bufferMemory_;
	unsigned char *bufferExtern_[MAX_EXTERN];
	int blockNum_;
	int noteNum_;
#ifdef SUPPORT_MULTI_THREAD
	CRITICAL_SECTION cs_block;
#endif
};
class MemBlock
{
public:
	MemBlock(unsigned char *buffer,long unitSize ,long noteNum);
	virtual ~MemBlock();
public:
	MemBlock *GetNextBlock();
	long GetFreeNoteNum();
	MemNote *GetNoteHead();
	void SetNextBlock(MemBlock *nextBlock){nextBlock_ = nextBlock;};
	void SetHeadNote(MemNote *headNote){headNote_ = headNote;};
	void SetUseNote(MemNote *headNote,bool bUsed);
	long GetUnitSize(){return unitSize_;};
private:
	MemBlock *nextBlock_;
	MemNote *headNote_;
	long freeNoteNum_;
	long totalNoteNum_;
	long unitSize_;
	unsigned char *bufferMemory_;
};
class MemNote
{
public:
	MemNote(unsigned char *noteMemory,int unitSize);
	virtual ~MemNote();
	friend class MemBlock;
	friend class MemPool;
public:
	unsigned char* GetBuffer();
	long GetSize();
	
private:
	void SetNextNote(MemNote *pNextNote){pNextNote_= pNextNote;};
	bool IsFree();
	void SetFree(bool IsFree);
	MemNote *GetNextNote();
	
private:
	unsigned char *noteMemory_;
	MemNote *pNextNote_;
	bool isFree_;
	long unitSize_;
};
#endif
