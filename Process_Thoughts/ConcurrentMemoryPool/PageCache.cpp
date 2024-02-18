#include "PageCache.h"

PageCache PageCache::_sInit;

Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k < NPAGES);

	// �ȼ���k��Ͱ��û��span
	if (!_spanList[k].Empty())
	{
		return _spanList[k].PopFront();
	}
	
	// �������Ͱ��û��span������У�������з�
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanList[i].Empty())
		{
			// �зֳ�һ��kҳ��span��һ��n-kҳ��span
			// kҳ��span���ظ�Central Cache��n-kҳ��span�ҵ�n-k��Ͱ��ȥ
			Span* nSpan = _spanList[i].PopFront();
			Span* kSpan = new Span;
			
			// ��nSpan��ͷ����һ��kҳ����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;
			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanList[nSpan->_n].PushFront(nSpan);

			return kSpan;
		}
	}

	// �ߵ�����Page Cache��û�з���Ҫ���Span������Ҫ���Ҫ�ڴ棬��ֱ��Ҫһ������Page
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	// ���������֮һ�ʣ�����ǰ��Ĵ���
	_spanList[bigSpan->_n].PushFront(bigSpan);
	return NewSpan(k);
}
