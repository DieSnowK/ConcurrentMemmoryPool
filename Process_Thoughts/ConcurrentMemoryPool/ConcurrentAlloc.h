#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

// Ϊʲô��Ҫ���������
// �ܲ�����ÿ���߳�����֮���Լ�ȥ����һ��ThreadCache��:P
static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeAlignMap::RoundUp(size);
		size_t kPage = alignSize >> PAGE_SHIFT;

		PageCache::GetInstance()->_pageMtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kPage);
		span->_objSize = size;
		PageCache::GetInstance()->_pageMtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			//pTLSThreadCache = new ThreadCache;
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();
		}

#ifdef DEBUG
		cout << std::this_thread::get_id() << " " << pTLSThreadCache << endl;
#endif

		return pTLSThreadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr); // ����ֻ����if���棬��Ȼ���������
	size_t size = span->_objSize;

	if (size > MAX_BYTES)
	{

		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();
	}
	else
	{
		assert(pTLSThreadCache && ptr);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}