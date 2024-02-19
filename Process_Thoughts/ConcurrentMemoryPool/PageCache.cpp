#include "PageCache.h"

PageCache PageCache::_sInit;

Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0);

	// ����128Pageֱ���������
	if (k > NPAGES - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		_idSpanMap[span->_pageId] = span;

		return span;
	}

	// �ȼ���k��Ͱ��û��span
	if (!_spanList[k].Empty())
	{
		Span* kSpan = _spanList[k].PopFront();

		// �˴�ҲҪӳ��id��span�����bug�ˡ�
		for (PAGE_ID i = 0; i < kSpan->_n; i++)
		{
			_idSpanMap[kSpan->_pageId + i] = kSpan;
		}

		return kSpan;
	}
	
	// �������Ͱ��û��span������У�������з�
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanList[i].Empty())
		{
			// �зֳ�һ��kҳ��span��һ��n-kҳ��span
			// kҳ��span���ظ�Central Cache��n-kҳ��span�ҵ�n-k��Ͱ��ȥ
			Span* nSpan = _spanList[i].PopFront();
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New();
			
			// ��nSpan��ͷ����һ��kҳ����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;
			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanList[nSpan->_n].PushFront(nSpan);
			// �洢nSpan����βҳ�Ÿ�nSpan��ӳ�䣬����Page Cache�����ڴ�ʱ���еĺϲ�����
			// TIPS��������ʱΪ�˷�����ǰ&���:P
			_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;

			// ����id��span��ӳ�䣬����Central Cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_n; i++)
			{
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	// �ߵ�����Page Cache��û�з���Ҫ���Span������Ҫ���Ҫ�ڴ棬��ֱ��Ҫһ������Page
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New();
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	// ���������֮һ�ʣ�����ǰ��Ĵ���
	_spanList[bigSpan->_n].PushFront(bigSpan);
	return NewSpan(k);
}

Span* PageCache::MapObjToSpan(void* obj)
{
	assert(obj);
	
	std::unique_lock<std::mutex> lock(_pageMtx); // RAII���������������Զ�����

	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	else
	{
		// ��Ӧ�����ߵ��⣬��Ϊobj��PAGE_IDӦ����ӳ���ϵ�
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// ����128Page��ֱ�ӻ�����
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);

		return;
	}

	// ��spanǰ���ҳ�����Խ��кϲ��������ڴ���Ƭ����
	// ��������ֶ�_isUse�жϣ�������_useCount�жϣ������̰߳�ȫ����
	while (1) // ��ǰ
	{
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = _idSpanMap.find(prevId);

		// ǰ���ҳ��û�У������кϲ�
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* prevSpan = ret->second;

		// ǰ������ҳ��span��ʹ�ã������кϲ�
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// �ϲ�������128ҳ��spanû�취����
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		//delete prevSpan;
		_spanPool.Delete(prevSpan);
	}

	while (1) // ��ǰ
	{
		PAGE_ID nextId = span->_pageId + span->_n;
		auto ret = _idSpanMap.find(nextId);

		// ǰ���ҳ��û�У������кϲ�
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* nextSpan = ret->second;

		// ǰ������ҳ��span��ʹ�ã������кϲ�
		if (nextSpan->_isUse == true)
		{
			break;
		}

		// �ϲ�������128ҳ��spanû�취����
		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_n += nextSpan->_n;

		_spanList[nextSpan->_n].Erase(nextSpan);
		//delete nextSpan;
		_spanPool.Delete(nextSpan);
	}

	_spanList[span->_n].PushFront(span);
	span->_isUse = false;
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_n - 1] = span;
}
