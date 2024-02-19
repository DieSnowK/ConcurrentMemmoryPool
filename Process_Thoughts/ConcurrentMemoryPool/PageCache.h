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

	// ��ȡPAGE_ID��Span*��ӳ��
	Span* MapObjToSpan(void* obj);

	// �ͷſ���span�ص�PageCache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);
private:
	PageCache()
	{}

	PageCache(const PageCache&) = delete;
private:
	SpanList _spanList[NPAGES];
	static PageCache _sInit;
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;
public:
	// ��Ҫ��һ�Ѵ���������ʹ��Ͱ��
	// ��Ϊ���������߳�ͬʱ�����һ��span��Ȼ���з�
	// ��ʱͰ����������̰߳�ȫ����
	// ���Ҵ�ʱͰ��������Ӱ��Ч��
	std::mutex _pageMtx;
};