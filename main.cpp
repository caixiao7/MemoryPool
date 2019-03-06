#include "MemPool.h"
#include <stdio.h>
#include <iostream>
MemPool mempool;
unsigned int __stdcall ProcClient( LPVOID para )
{
	while (1)
	{
		MemNote *pCurNote;
		pCurNote = mempool.GetMemoryNote();
		if (pCurNote == NULL)
		{
			return 0;
		}
		Sleep(600);
		unsigned char *p = pCurNote->GetBuffer();
		memset(p,1,pCurNote->GetSize());
		
	
		printf("thread1 %d",*(pCurNote->GetBuffer()));

		printf("\n");
		mempool.ReleaseMemory(pCurNote);
	
		//sdadfsadf  
		//mempool.DestoryPool();
	}

	return 1;
}
unsigned int __stdcall ProcClient2( LPVOID para )
{
	while(1)
	{
		MemNote *pCurNote;
		pCurNote = mempool.GetMemoryNote();
		if (pCurNote == NULL)
		{
			return 0;
		}
Sleep(500);
		unsigned char *p = pCurNote->GetBuffer();
		memset(p,2,pCurNote->GetSize());

		printf("thread1 %d",*(pCurNote->GetBuffer()));

		printf("\n");
		mempool.ReleaseMemory(pCurNote);


		
		//mempool.DestoryPool();
	}
	return 1;
}
int main()
{
	if(!mempool.InitPool())
		return 1;
	
	UINT nThreadID1,nThreadID2;
	HANDLE hand_procThread = ( HANDLE )_beginthreadex( NULL, 0, ProcClient, ( void * )( NULL ), 0, &nThreadID1 );
	CloseHandle(hand_procThread);


	HANDLE hand_procThread2 = ( HANDLE )_beginthreadex( NULL, 0, ProcClient2, ( void * )( NULL ), 0, &nThreadID2 );
	CloseHandle(hand_procThread2);
	
	
	while (1)
	{
		std::cout<<mempool.GetFreeMemorySize()/(2*1024*1024);
		Sleep(500);
	}
	
		
	return 0;
}