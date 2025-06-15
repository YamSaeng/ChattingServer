#pragma once
#include<stdlib.h>
#include<string.h>
#include<new.h>
#include<Windows.h>

//==========================================================
// ObjectPool
//==========================================================
// 노드들을 내부적으로 스택 형식으로 관리하고
// 사용중인 노드 개수와 총 노드 개수를 비교해서
// 스택에 노드가 있으면 반환해주고 없다면 새로 할당하여 반환해준다.
//==========================================================

template <class DATA>
class ObjectPool
{
private:
#pragma pack(push,1)
	struct Node
	{
		DATA Data;		 // 실제 데이터
		Node* NextBlock; // 블록들을 리스트 형태로 관리 하고 있기에 다음 블록의 주소를 담아둘 변수
	};
#pragma pack(pop)
#pragma pack(push,1)
	struct CheckNode
	{
		Node* TopNode;
		LONG64 NodeCheckValue;    // LockfreeStack 체크할때 쓰일 변수값
	};
#pragma pack(pop)
	CheckNode* _TopNode;		   // 스택의 Top과 동일한 기능
	LONG _AllocCount;			   // 총 블럭 개수
	LONG _UseCount;				   // 사용 중인 블럭 개수
	LONG64 _LockFreeCheckCount;    // 락프리 체크용 카운트

	bool _IsPlacementNew;
public:
	ObjectPool();
	virtual ~ObjectPool();

	DATA* Alloc(void);
	bool Free(DATA* Data);

	int GetAllocCount(void);
	int GetUseCount(void);
	bool IsAlloc();
};

template<class DATA>
ObjectPool<DATA>::ObjectPool()
{
	_TopNode = (CheckNode*)_aligned_malloc(sizeof(CheckNode), 16);
	_TopNode->TopNode = nullptr;	
	_TopNode->NodeCheckValue = 0;
	_AllocCount = 0;
	_UseCount = 0;
	_LockFreeCheckCount = 0;
}

template<class DATA>
ObjectPool<DATA>::~ObjectPool()
{
	if (_TopNode != nullptr)
	{
		Node* FreeNode = _TopNode->TopNode;

		if (FreeNode != nullptr)
		{
			for (int i = 0; i < _AllocCount; i++)
			{
				char* p = (char*)FreeNode;
				char* q = p;

				DATA* FreeData = (DATA*)p;
				FreeData->~DATA();
				FreeNode = FreeNode->NextBlock;

				free(q);
			}
		}

		_aligned_free(_TopNode);
		_TopNode = nullptr;
	}
}

//----------------------------------------------------------
// 새로운 노드 할당
// 사용 할 수 있는 블럭이 없을경우 새로 할당하여 반환해주고
// 사용 할 수 있는 블럭이 있을경우 보관중인 블럭을 반환해준다.
//----------------------------------------------------------  
template<class DATA>
DATA* ObjectPool<DATA>::Alloc(void)
{
	Node* AllocNode;
	CheckNode Top;
	int AllocCount = _AllocCount;
	int UseCount = _UseCount;

	InterlockedIncrement(&_UseCount);

	if (AllocCount > _UseCount) //사용 할 수 있는 블럭이 있는 경우
	{
		LONG64 LockFreeCheckCount = InterlockedIncrement64(&_LockFreeCheckCount);

		do
		{
			Top.NodeCheckValue = _TopNode->NodeCheckValue;
			Top.TopNode = _TopNode->TopNode;

			AllocNode = Top.TopNode;
		} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, LockFreeCheckCount, (LONG64)_TopNode->TopNode->NextBlock, (LONG64*)&Top));

		new (AllocNode) DATA;
	}
	else //사용 할 수 있는 블럭이 없을 경우
	{
		//새로운 노드 할당해주고 생성자 호출 후 넘겨줌

		AllocNode = (Node*)malloc(sizeof(Node));
		memset(AllocNode, 0, sizeof(Node));
		new (AllocNode) DATA;

		/*
			할당 수와 사용 수를 늘려준다.
		*/
		InterlockedIncrement(&_AllocCount);
	}

	return (DATA*)&AllocNode->Data;
}

//-----------------------------------------------
// 사용 다한 블럭을 반환한다.
//-----------------------------------------------
template<class DATA>
bool ObjectPool<DATA>::Free(DATA* Data)
{
	/*
		받은 데이터를 풀에 넣기 ( Push )
	*/
	CheckNode Top;
	Node* FreeNode = (Node*)Data;

	do
	{
		Top.NodeCheckValue = _TopNode->NodeCheckValue;
		Top.TopNode = _TopNode->TopNode;

		FreeNode->NextBlock = Top.TopNode;
	} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, Top.NodeCheckValue, (LONG64)FreeNode, (LONG64*)&Top));

	InterlockedDecrement(&_UseCount);

	return true;
}

template<class DATA>
int ObjectPool<DATA>::GetAllocCount(void)
{
	return _AllocCount;
}

template<class DATA>
int ObjectPool<DATA>::GetUseCount(void)
{
	return _UseCount;
}

template<class DATA>
bool ObjectPool<DATA>::IsAlloc()
{
	return false;
}
