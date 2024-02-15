#pragma once
#include <iostream>

#ifdef _WIN32
	#include <Windows.h>
#elif
	// Linux�µ�ͷ�ļ�
#endif

// ֱ��ȥ���ϰ�ҳ����ռ䣬���װ���malloc(��Ҫ)������Ϊ�����Ч��
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

// �����ڴ��v1.0
//template<size_t N>
//class ObjectPool
//{};

// ��Ϊÿ�������С�ǹ̶��ģ�����Ҳ�����������
template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		// �������Ѿ����������ڴ��
		if (_freeList)
		{
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;
		}
		else
		{
			// ʣ���ֽ�������һ�������С�������������ռ�
			if (_remainBytes < sizeof(T))
			{
				_remainBytes = 128 * 1024;
				_memory = (char*)SystemAlloc(_remainBytes >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}

			obj = (T*)_memory;

			// ����T�����СС��һ��ָ�룬��ʱ��ֻʹ��һ��T�Ĵ�С����治��һ����ַ
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T); 
			_memory += sizeof(T);
			_remainBytes -= sizeof(T);
		}

		// ��λnew����ʽ����T�Ĺ��캯����ʼ��
		// new (address) type
		new(obj)T;

		return obj;
	}

	void Delete(T* obj)
	{
		// ��ʽ�������������������
		obj->~T();

		// ͷ��
		//*(int*)obj = _freeList; // ���ﲻ�������ã���Ϊx32��x64����£�ָ���С��ͬ
		*(void**)obj = _freeList;
		_freeList = obj;

	}
private:
	char* _memory = nullptr; // ָ�����ڴ��ָ��
	void* _freeList = nullptr; // ���������ڴ�����������ͷָ��
	size_t _remainBytes = 0; // ����ڴ����зֹ�����ʣ���ֽ��������ڷ�ֹԽ��
};