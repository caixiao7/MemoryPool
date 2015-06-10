#include "MemPool.h"
#include <stdio.h>

void WriteToLog(unsigned char *logInfo)
{
	printf("%s",logInfo);
}
MemPool::MemPool()
{
#ifdef SUPPORT_MULTI_THREAD
	InitializeCriticalSection(&cs_block);
#endif
	blockHead_ = NULL;
	memorySize_ = 0;
	freeSize_ = 0;
	unitSize_ = 0;
	bufferMemory_ = NULL;
	for(int i=0;i<MAX_EXTERN;i++)
	{
		bufferExtern_[i] = NULL;
	}
}
MemPool::~MemPool()
{
#ifdef SUPPORT_MULTI_THREAD
	DeleteCriticalSection(&cs_block);
#endif
}
/*
*InitPool:
*初始化内存池，根据block和note（相当于矩阵中的行列），通过列表形式，
*将申请的内存分配到各个Note
*/
bool MemPool::InitPool(int blockNum, int noteNum, long unitSize)
{
	if (bufferMemory_ != NULL)
		return true;
	if (unitSize > MAX_MEMORYSIZE)
	{
		WriteToLog((unsigned char*)"please reset unit size\n");
		return false;
	}
	bufferMemory_ = new unsigned char[blockNum * noteNum * unitSize];
	if (bufferMemory_ == NULL)
	{
		WriteToLog((unsigned char*)"apply for memory failed\n");
		return false;
	}
	MemBlock *pPrevBlock;
	for(int i = 0;i<blockNum;i++)
	{
		// 在析构的时候将内存池链接释放，防止内存泄露
		MemBlock *pCurBlock = new MemBlock(bufferMemory_ + i * noteNum * unitSize, unitSize, noteNum);

		if (i == 0)
			blockHead_ = pCurBlock;
		else
			pPrevBlock->SetNextBlock(pCurBlock);

		pPrevBlock = pCurBlock;

		MemNote *pPrevNote;
		for (int j = 0;j<noteNum;j++)
		{
			// 在析构的时候将内存池链接释放，防止内存泄露
			MemNote *pCurNote = new MemNote(bufferMemory_ + i * noteNum * unitSize + j * unitSize,unitSize);

			if (j == 0)
				pCurBlock->SetHeadNote(pCurNote);
			else
				pPrevNote->SetNextNote(pCurNote);

			pPrevNote = pCurNote;
		}
		pPrevNote->SetNextNote(NULL);
	}
	pPrevBlock->SetNextBlock(NULL);
	blockNum_ = blockNum;
	noteNum_ = noteNum;
	unitSize_ = unitSize;
	memorySize_ = unitSize_ * blockNum_ * noteNum_;
	freeSize_ = memorySize_;
	return true;
}
/*在initPool之后调用，释放每个Block以及内存
*
*/
void MemPool::DestoryPool()
{
	MemBlock *pCurBlock = blockHead_;
	while (pCurBlock != NULL)
	{
		MemNote *pCurNote = pCurBlock->GetNoteHead();
		while (pCurNote != NULL)
		{
			MemNote *pDelNote = pCurNote;
			pCurNote = pCurNote->GetNextNote();
			
			delete pDelNote;
			if (pCurBlock->GetNoteHead() != NULL)
			{
				pCurBlock->SetHeadNote(NULL);
			}
		}
		MemBlock *pDelBlock = pCurBlock;
		pCurBlock = pCurBlock->GetNextBlock();

		delete pDelBlock;
		if (blockHead_ != NULL)
		{
			blockHead_= NULL;
		}
		

	}
	//释放初始化申请的内存
	if (bufferMemory_ != NULL)
	{
		delete bufferMemory_;
		bufferMemory_ = NULL;
	}
	//释放扩展的内存
	for (int i=0;i<MAX_EXTERN;i++)
	{
		if (bufferExtern_[i] != NULL)
		{
			delete bufferExtern_[i];
			bufferExtern_[i] = NULL;
		}
	}
	memorySize_= 0;
	freeSize_ = memorySize_;

	
}
/*
*在初始化内存中获取内存Note
*/
MemNote *MemPool::GetMemoryNote()
{
#ifdef SUPPORT_MULTI_THREAD
	EnterCriticalSection(&cs_block);
#endif
	MemBlock *pCurBlock = blockHead_;
	while (pCurBlock != NULL)
	{
		while (pCurBlock->GetFreeNoteNum() <= 0)
		{
			pCurBlock = pCurBlock->GetNextBlock();
			if (pCurBlock == NULL)
			{
#ifdef SUPPORT_MULTI_THREAD
				LeaveCriticalSection(&cs_block);
#endif
				return NULL;
			}
				
		}
		MemNote *pCurNote = pCurBlock->GetNoteHead();
		while (pCurNote != NULL)
		{
			if (pCurNote->IsFree())
			{
				freeSize_ -= pCurNote->GetSize();
				pCurBlock->SetUseNote(pCurNote,true);
				WriteToLog((unsigned char*)"apply for memory success\n");
#ifdef SUPPORT_MULTI_THREAD
				LeaveCriticalSection(&cs_block);
#endif
				return pCurNote;
			}
			pCurNote = pCurNote->GetNextNote();
		}
		WriteToLog((unsigned char*)"no enough memory in pool\n");
#ifdef SUPPORT_MULTI_THREAD
		LeaveCriticalSection(&cs_block);
#endif
		return NULL;
	}
	WriteToLog((unsigned char*)"no enough memory in pool\n");
#ifdef SUPPORT_MULTI_THREAD
	LeaveCriticalSection(&cs_block);
#endif
	return NULL;
}
/*
*当memorySize大于初始化的内存单元，则先扩展内存中是否有符合要求的Note，否则申请新的内存，
*添加到Block尾部
*/
MemNote *MemPool::GetMemoryNote(long memorySize)
{
	if (memorySize > unitSize_)
	{
#ifdef SUPPORT_MULTI_THREAD
		EnterCriticalSection(&cs_block);
#endif
		MemBlock *pCurBlock = blockHead_;
		while (pCurBlock != NULL)
		{
			// 当已经添加了比默认内存单元还大的Block时，直接取来用。
			if( pCurBlock->GetUnitSize() >= memorySize && pCurBlock->GetFreeNoteNum() > 0)
			{
				MemNote *pCurNote = pCurBlock->GetNoteHead();
				while (pCurNote != NULL)
				{
					if (pCurNote->IsFree())
					{
						freeSize_ -= pCurNote->GetSize();
						pCurBlock->SetUseNote(pCurNote,true);
						WriteToLog((unsigned char*)"apply for memory success\n");
						return pCurNote;
					}
					pCurNote = pCurNote->GetNextNote();
				}
				WriteToLog((unsigned char*)"unexpected error: memory not found");
				pCurBlock->GetFreeNoteNum();
				return NULL;
			}
			//最后一个block 跳出while
			if (pCurBlock->GetNextBlock() != NULL)
			{
				pCurBlock = pCurBlock->GetNextBlock();
			}
			else
				break;
			
		}

		//内存池中没有符合条件的Block，增加新的Block

		//1.在扩展buffer中找出没有用到的
		int index = 0;
		for(;index < MAX_EXTERN;index ++)
		{
			if (bufferExtern_[index] == NULL)
				break;
			if (index == MAX_EXTERN-1 )
			{
				WriteToLog((unsigned char*)"error: not enough extern buffer\n");
				return NULL;
			}
		}
		//2.申请内存
		bufferExtern_[index] = new unsigned char[noteNum_ * memorySize];
		if (bufferExtern_[index] == NULL)
		{
			WriteToLog((unsigned char *)"apply for memory failed");
			return NULL;
		}
		memorySize_ += noteNum_ * memorySize;
		freeSize_ += noteNum_ * memorySize;
		MemBlock *pNewBlock = new MemBlock(bufferExtern_[index], memorySize, noteNum_);
		
		pCurBlock->SetNextBlock(pNewBlock);

		MemNote *pPrevNote;
		for (int j = 0;j<noteNum_;j++)
		{
			// 在析构的时候将内存池链接释放，防止内存泄露
			MemNote *pCurNote = new MemNote(bufferExtern_[index] + j * memorySize,memorySize);

			if (j == 0)
				pNewBlock->SetHeadNote(pCurNote);
			else
				pPrevNote->SetNextNote(pCurNote);

			pPrevNote = pCurNote;
		}
		pPrevNote->SetNextNote(NULL);
		pNewBlock->SetNextBlock(NULL);
		blockNum_++;
		freeSize_ -= pNewBlock->GetNoteHead()->GetSize();
		pNewBlock->SetUseNote(pNewBlock->GetNoteHead(),true);
		WriteToLog((unsigned char*)"apply for memory success\n");
#ifdef SUPPORT_MULTI_THREAD
		LeaveCriticalSection(&cs_block);
#endif
		return pNewBlock->GetNoteHead();
	}
	return GetMemoryNote();
	
}
/*
*当外部使用者归还内存到内存池时，调用ReleaseMemory
*/
void MemPool::ReleaseMemory(MemNote *pMemoryNote)
{
#ifdef SUPPORT_MULTI_THREAD
	EnterCriticalSection(&cs_block);
#endif
	MemBlock *pCurBlock = blockHead_;
	while (pCurBlock != NULL)
	{
		MemNote *pCurNote = pCurBlock->GetNoteHead();
		while (pCurNote != NULL)
		{
			if (pMemoryNote == pCurNote)
			{
				freeSize_ += pMemoryNote->GetSize();
				pCurBlock->SetUseNote(pMemoryNote,false);
#ifdef SUPPORT_MULTI_THREAD
				LeaveCriticalSection(&cs_block);
#endif
				return;
			}
			pCurNote = pCurNote->GetNextNote();
		}
		pCurBlock = pCurBlock->GetNextBlock();
		
	}
#ifdef SUPPORT_MULTI_THREAD
	LeaveCriticalSection(&cs_block);
#endif
	WriteToLog((unsigned char*)"release memory failed\n");
}
long double MemPool::GetTotalMemorySize()
{
	return memorySize_ - freeSize_;
}
long double MemPool::GetFreeMemorySize()
{
	return freeSize_;
}





MemBlock::MemBlock(unsigned char *buffer,long unitSize ,long noteNum)
{
	bufferMemory_ = buffer;
    unitSize_ = unitSize;
	totalNoteNum_ = noteNum;
	freeNoteNum_ = totalNoteNum_;
}
MemBlock::~MemBlock()
{

}

MemBlock *MemBlock::GetNextBlock()
{
	return nextBlock_;
}
long MemBlock::GetFreeNoteNum()
{
	return freeNoteNum_;
}
MemNote *MemBlock::GetNoteHead()
{
	return headNote_;
}
void MemBlock::SetUseNote(MemNote *headNote,bool bUsed)
{
	headNote->SetFree(!bUsed);
	if (bUsed)
		freeNoteNum_--;
	else
		freeNoteNum_++;
}






MemNote::MemNote(unsigned char *noteMemory,int unitSize)
{
	noteMemory_ = noteMemory;
	unitSize_ = unitSize;
}
MemNote::~MemNote()
{

}
unsigned char* MemNote::GetBuffer()
{
	return noteMemory_;
}
long MemNote::GetSize()
{
	return unitSize_;
}
bool MemNote::IsFree()
{
	return isFree_;
}
void MemNote::SetFree(bool IsFree)
{
	if (IsFree == true)
	{
		memset(noteMemory_,0x00,unitSize_);
	}
	isFree_ = IsFree;
}
MemNote *MemNote::GetNextNote()
{
	return pNextNote_;
}

