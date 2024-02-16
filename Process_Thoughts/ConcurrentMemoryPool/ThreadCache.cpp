#include "ThreadCache.h"

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
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
	return nullptr;
}
