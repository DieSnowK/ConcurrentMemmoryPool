#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInit;

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t alignSize)
{
	size_t index = SizeAlignMap::Index(alignSize);
	_spanLists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanLists[index], alignSize);
	assert(span);
	assert(span->_freeList);

	// ��span�л�ȡbatchNum������
	// �������batchNum�����ж����ö���
	start = end = span->_freeList;
	size_t actualNum = 1;
	for (size_t i = 0; i < batchNum - 1 && NextObj(end); i++)
	{
		end = NextObj(end);
		actualNum++;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_useCount += actualNum;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

// �ȴ�SpanList�б���Ѱ�ҷ���Ҫ���Span����û�У����PageCache��ȡ
Span* CentralCache::GetOneSpan(SpanList& list, size_t alignSize)
{
	// �鿴��ǰ��spanList���Ƿ��л�δ��������span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	// �Ȱ�Central Cache��Ͱ���������������߳��ͷ��ڴ�����������������
	list._mtx.unlock();

	// ���ˣ�˵��û�п��е�span�ˣ���Ҫ��PageCache��ȡ
	// NewSpan�еݹ�����һ�ֽ������|
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeAlignMap::MovePageNum(alignSize));
	PageCache::GetInstance()->_pageMtx.unlock();
	// ����Ҫ��������Central Cache��Ͱ��,��Ϊkҳspanֻ�е�ǰ�߳����õ��������߳��ò���

	// ��ȡspan֮����Ҫ�з�span������Ҫ����
	// ����span�Ĵ���ڴ���ʼ��ַ�ʹ���ڴ�Ĵ�С(�ֽ���)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes; 

	// �Ѵ���ڴ��г�����������������
	// β��ȽϺã��������������ģ�������CPU���������ʱȽϸ�
	// 1.����һ������ȥ��ͷ������β��
	span->_freeList = start;
	start += alignSize;
	void* tail = span->_freeList;

	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += alignSize;
	}

	// �к�span�Ժ���Ҫ��span�ҵ�Ͱ����ȥ��ʱ���ټ���
	list._mtx.lock();
	// �󲿷�����£��µ�span���궼����ʣ�����Խ�span��SpanList
	list.PushFront(span);											

	return span;
}
