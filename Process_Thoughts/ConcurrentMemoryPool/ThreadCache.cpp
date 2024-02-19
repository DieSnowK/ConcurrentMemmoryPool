#include "ThreadCache.h"
#include "CentralCache.h"

// ���ù����������// TODO

void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeAlignMap::RoundUp(size);
	size_t index = SizeAlignMap::Index(size);

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		// ��CentralCache��ȡ�ڴ�
		return FetchFromCentralCache(index, alignSize);
	}
}

// ������Ҫ����size��free�ǲ���Ҫsize��
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size <= MAX_BYTES);
	assert(ptr);
	
	// �ҳ�ӳ�����������Ͱ��ͷ���ȥ
	size_t index = SizeAlignMap::Index(size);
	_freeLists[index].Push(ptr);

	// �������ȴ���һ������������ڴ�ʱ���Ϳ�ʼ����һ�θ�Central Cache
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
	// ����ʼ���������㷨
	// 1.�ʼ����һ������Central CacheҪ̫�࣬��Ϊ����Ҫ̫���ò���
	// 2.���������alignSize��С�ڴ��������ôbatchNum�ͻ᲻��������ֱ������
	// 3.alignSizeԽ��һ������Central CacheҪ��batchNum��ԽС
	// 4.alignSizeԽС��һ������Central CacheҪ��batchNum��Խ��	
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeAlignMap::MoveObjNum(alignSize));
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 2;
	}

	void* start = nullptr;
	void* end = nullptr;

	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);
	assert(actualNum > 0);
	
	if (actualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else
	{
		// ����ͷ�ϵ�һ����ʣ�µĹ���FreeList��
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;

	list.PopRange(start, end, list.MaxSize());

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}

