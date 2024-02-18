#pragma once
#include "Common.h"

// ����
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInit;
	}

	// ��ȡһ��kҳ��span
	Span* NewSpan(size_t k);
private:
	PageCache()
	{}

	PageCache(const PageCache&) = delete;
private:
	SpanList _spanList[NPAGES];
	static PageCache _sInit;
public:
	// ��Ҫ��һ�Ѵ���������ʹ��Ͱ��
	// ��Ϊ���������߳�ͬʱ�����һ��span��Ȼ���з�
	// ��ʱͰ����������̰߳�ȫ����
	// ���Ҵ�ʱͰ��������Ӱ��Ч��
	std::mutex _pageMtx;
};