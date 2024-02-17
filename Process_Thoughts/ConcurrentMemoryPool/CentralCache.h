#pragma once
#include "Common.h"

// ����
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInit;
	}

	// ��Central Cache��ȡһ�������Ķ����Thread Cache
	// start, end ����Ͳ��������ص�ַ
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t alignSize);

	// ��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t size);
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;
private:
	SpanList _spanLists[NFREELIST];
	static CentralCache _sInit;
};