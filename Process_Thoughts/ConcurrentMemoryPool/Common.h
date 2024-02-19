#pragma once
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <cassert>
using std::cout;
using std::endl;

#ifdef _WIN32
	#include <Windows.h>
#else
	// Linux
#endif

#define DEBUG

// ����������� // TODO
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	// Linux
#endif

static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208; // ��ϣͰ�ĸ���
static const size_t NPAGES = 129; // ���Page�ĸ�����128������0�±�ռλһ��
static const size_t PAGE_SHIFT = 13; // �ֽ�����ҳ��ת��

// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// Linux��brk mmap��
#endif

	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	return ptr;
}

// staticʹNextObj()ֻ�ڵ�ǰ.cpp�ļ��пɼ�����Ϊ����ļ��������Common.h
static inline void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// �����зֺõ�С�������������
class FreeList
{
public:
	void Push(void* obj)
	{
		assert(obj);

		// ͷ��
		NextObj(obj) = _freeList; //*(void**)obj = _freeList;
		_freeList = obj;
		_size++;
	}
	void PushRange(void* start, void* end, size_t num)
	{
		assert(start);
		assert(end);

		NextObj(end) = _freeList;
		_freeList = start;
		_size += num;
	}

	void* Pop()
	{
		assert(_freeList);

		// ͷɾ
		void* obj = _freeList;
		_freeList = NextObj(obj);
		_size--;

		return obj;
	}

	void PopRange(void*& start, void*& end, size_t n)
	{
		assert(n >= _size);

		start = _freeList;
		end = start;

		for (size_t i = 0; i < n - 1; i++)
		{
			end = NextObj(end);
		}
		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}

	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t Size()
	{
		return _size;
	}

	size_t& MaxSize()
	{
		return _maxsize;
	}
private:
	void* _freeList = nullptr;
	size_t _maxsize = 1;
	size_t _size = 0;
};

// ˼��Ϊʲô������8Bytes���룿
// ��Ҫ32768��8Bytes��������������˷��ڴ�

// ������������10%���ҵ�����Ƭ�˷�
// [1,128]					8byte����	    freelist[0,16)
// [128+1,1024]				16byte����	    freelist[16,72)
// [1024+1,8*1024]			128byte����	    freelist[72,128)
// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)

// ��������С�Ķ���ӳ�����
class SizeAlignMap
{
public:
	// v1.0
	//static inline size_t _RoundUp(size_t size, size_t alignNum)
	//{
	//	size_t alignSize = size;
	//	if (size % 8 != 0)
	//	{
	//		alignSize = (size / alignNum + 1) * alignNum; // ���϶��룬����+1
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}

	// v2.0
	static inline size_t _RoundUp(size_t size, size_t alignNum)
	{
		return (size + alignNum - 1) & ~(alignNum - 1);
	}


	// �������϶����Ƕ���
	static inline size_t RoundUp(size_t size) 
	{
		assert(size <= MAX_BYTES);

		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8 * 1024);
		}
		else
		{
			// ������˵�����ߵ�����
			assert(false);
			return -1;
		}
	}

	// v1.0
	//static inline size_t _Index(size_t size, size_t alignNum)
	//{
	//	if (size % alignNum == 0)
	//	{
	//		return size / alignNum - 1;
	//	}
	//	else
	//	{
	//		return size / alignNum;
	//	}
	//}

	static inline size_t _Index(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	// ����ӳ�����һ����������Ͱ
	static inline size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);
		static const size_t FrontNum[4] = {16, 56, 56, 56};

		if (size <= 128)
		{
			return _Index(size, 3);
		}
		else if (size <= 1024)
		{
			return _Index(size, 4) + FrontNum[0];
		}
		else if (size <= 8 * 1024)
		{
			return _Index(size, 7) + FrontNum[0] + FrontNum[1];
		}
		else if (size <= 64 * 1024)
		{
			return _Index(size, 10) + FrontNum[0] + FrontNum[1] + FrontNum[2];
		}
		else if (size <= 256 * 1024)
		{
			return _Index(size, 13) + FrontNum[0] + FrontNum[1] + FrontNum[2] + FrontNum[3];
		}
		else
		{
			// ������˵�����ߵ�����
			assert(false);
			return -1;
		}
	}

	// Thread Cacheһ�δ�Central Cache��ȡ���ٸ�����
	static size_t MoveObjNum(size_t size)
	{
		assert(size <= MAX_BYTES);
		assert(size > 0);

		// [2, 512] һ�������ƶ����������ķ�Χ������һ���Ի�ö������������̫�࣬Ҳ����̫��
		// С�������޸ߣ���������޵�
		size_t num = MAX_BYTES / size;
		if (num < 2)
		{
			num = 2;
		}

		if (num > 512)
		{
			num = 512;
		}

		return num;
	}

	// ����һ������ϵͳ��ȡ����ҳ
	// ÿ�������С��ͬ����ȡ��������ͬ
	// ��������8Byte VS ��������256KB
	static size_t MovePageNum(size_t alignSize)
	{
		assert(alignSize > 0);

		size_t num = MoveObjNum(alignSize);
		size_t nPage = num * alignSize; // ���ֽ���
		nPage >>= PAGE_SHIFT; // ת��������ҳ��

		if (nPage == 0)
		{
			nPage = 1;
		}

		return nPage;
	}
private:
	
};

// ����������ҳ����ڴ��Ƚṹ
struct Span
{
	PAGE_ID _pageId = 0; // ����ڴ���ʼҳ��ҳ��
	size_t _n = 0; // ҳ������

	Span* _next = nullptr; // ˫������Ľṹ
	Span* _prev = nullptr;

	size_t _useCount = 0; // �к�С���ڴ棬�������Thread Cache�ļ���
	bool _isUse = false; // �Ƿ��ڱ�ʹ��
	void* _freeList = nullptr; // �к�С���ڴ����������
};

// ��ͷ˫������ʾ��ͼ���Կ����޸� // TODO
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}

	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;

	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}
private:
	Span* _head; // ��ͷ˫�������ͷ�ڵ�
public:
	std::mutex _mtx; // Ͱ��
};