#pragma once
#include<stdlib.h>
#include<string.h>
#include<new.h>
#include<Windows.h>

//==========================================================
// ObjectPool
//==========================================================
// ������ ���������� ���� �������� �����ϰ�
// ������� ��� ������ �� ��� ������ ���ؼ�
// ���ÿ� ��尡 ������ ��ȯ���ְ� ���ٸ� ���� �Ҵ��Ͽ� ��ȯ���ش�.
//==========================================================

template <class DATA>
class ObjectPool
{
private:
#pragma pack(push,1)
	struct Node
	{
		DATA Data;		 // ���� ������
		Node* NextBlock; // ��ϵ��� ����Ʈ ���·� ���� �ϰ� �ֱ⿡ ���� ����� �ּҸ� ��Ƶ� ����
	};
#pragma pack(pop)
#pragma pack(push,1)
	struct CheckNode
	{
		Node* TopNode;
		LONG64 NodeCheckValue;    // LockfreeStack üũ�Ҷ� ���� ������
	};
#pragma pack(pop)
	CheckNode* _TopNode;		   // ������ Top�� ������ ���
	LONG _AllocCount;			   // �� �� ����
	LONG _UseCount;				   // ��� ���� �� ����
	LONG64 _LockFreeCheckCount;    // ������ üũ�� ī��Ʈ

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
// ���ο� ��� �Ҵ�
// ��� �� �� �ִ� ���� ������� ���� �Ҵ��Ͽ� ��ȯ���ְ�
// ��� �� �� �ִ� ���� ������� �������� ���� ��ȯ���ش�.
//----------------------------------------------------------  
template<class DATA>
DATA* ObjectPool<DATA>::Alloc(void)
{
	Node* AllocNode;
	CheckNode Top;
	int AllocCount = _AllocCount;
	int UseCount = _UseCount;

	InterlockedIncrement(&_UseCount);

	if (AllocCount > _UseCount) //��� �� �� �ִ� ���� �ִ� ���
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
	else //��� �� �� �ִ� ���� ���� ���
	{
		//���ο� ��� �Ҵ����ְ� ������ ȣ�� �� �Ѱ���

		AllocNode = (Node*)malloc(sizeof(Node));
		memset(AllocNode, 0, sizeof(Node));
		new (AllocNode) DATA;

		/*
			�Ҵ� ���� ��� ���� �÷��ش�.
		*/
		InterlockedIncrement(&_AllocCount);
	}

	return (DATA*)&AllocNode->Data;
}

//-----------------------------------------------
// ��� ���� ���� ��ȯ�Ѵ�.
//-----------------------------------------------
template<class DATA>
bool ObjectPool<DATA>::Free(DATA* Data)
{
	/*
		���� �����͸� Ǯ�� �ֱ� ( Push )
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
