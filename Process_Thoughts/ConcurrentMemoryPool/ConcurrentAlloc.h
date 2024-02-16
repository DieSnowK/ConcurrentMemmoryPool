#pragma once
#include "ThreadCache.h"

// Ϊʲô��Ҫ���������
// �ܲ�����ÿ���߳�����֮���Լ�ȥ����һ��ThreadCache��:P
static void* ConcurrentAlloc(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}

#ifdef DEBUG
	cout << std::this_thread::get_id() << " " << pTLSThreadCache << endl;
#endif

	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{
	assert(pTLSThreadCache && ptr);

	pTLSThreadCache->Deallocate(ptr, size);
}