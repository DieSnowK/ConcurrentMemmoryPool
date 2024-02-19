#pragma once
#include "Common.h"
#include "ObjectPool.h"

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
	ObjectPool<Span> _spanPool;
	std::unordered_map<PAGE_ID, Span*> _idSpanMap; // ����ʱ��Ҫ������STL�����������̰߳�ȫ������
	//std::unordered_map<PAGE_ID, size_t> _idSizeMap; // ��ѡ����֮һ��������ӳ���ϵ

	static PageCache _sInit;
public:
	// ��Ҫ��һ�Ѵ���������ʹ��Ͱ��
	// ��Ϊ���������߳�ͬʱ�����һ��span��Ȼ���з�
	// ��ʱͰ����������̰߳�ȫ����
	// ���Ҵ�ʱͰ��������Ӱ��Ч��
	std::mutex _pageMtx;
};