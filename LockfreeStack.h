#pragma once

#include "ObjectPool.h"

template <class DATA>
class LockfreeStack
{
private:
	struct Node
	{
		DATA Data;		// ������ ������
		Node* NextNode; // ���� ��带 ����Ű�� ������
	};

	// ABA ���� �ذ��� ���� ����ü
	struct CheckNode
	{
		Node* TopNode; // �ֻ��� ��� ������
		LONG64 NodeCheckValue; // ABA ���� ������ ī���� ����
	};
	
	CheckNode* _TopNode; // Stack�� �ֻ��� ��带 ��Ƶ�

	LONG64 _LockFreeCheckCount;	// ABA ������ ī���� ����
	ObjectPool<Node>* _ObjectPool; // Node Ǯ
public:
	LockfreeStack();
	~LockfreeStack();

	LONG _LockFreeCount; // Stack�� �ִ� ������ ����

	void Push(DATA Data); // Stack�� ������ �߰�
	bool Pop(DATA* Data); // Stack���� ������ ������
};

template<class DATA>
LockfreeStack<DATA>::LockfreeStack()
{
	// ��� �޸� Ǯ ����
	_ObjectPool = new ObjectPool<Node>();
	// 16����Ʈ ���ĵ� �޸𸮷� �Ҵ� (128bit CAS�� ����)
	_TopNode = (CheckNode*)_aligned_malloc(sizeof(CheckNode), 16);

	// �ʱ� ���·� �ʱ�ȭ
	_TopNode->TopNode = nullptr;
	_TopNode->NodeCheckValue = 0;

	// ī���� �� �ʱ�ȭ
	_LockFreeCount = 0;
	_LockFreeCheckCount = 0;
}

template<class DATA>
inline LockfreeStack<DATA>::~LockfreeStack()
{

}

template<class DATA>
void LockfreeStack<DATA>::Push(DATA Data)
{
	CheckNode Top; // ���� Stack ���¸� �о�� �ӽ� ����
	Node* NewNode = _ObjectPool->Alloc(); // Ǯ���� �� ��� �Ҵ�
	NewNode->Data = Data; // ������ ����

	// CheckCount ����
	LONG64 LockFreeCheckCount = InterlockedIncrement64(&_LockFreeCheckCount);

	do
	{
		// ���� Stack�� TopNode�� ���������� ����
		Top.TopNode = _TopNode->TopNode;
		Top.NodeCheckValue = _TopNode->NodeCheckValue;

		// �� ��尡 �ֻ��� ��带 ����Ű���� ����
		NewNode->NextNode = _TopNode->TopNode;

		// _TopNode�� (NewNode, LockFreeCheckCount)�� ����
		// �����ϸ� �ٸ� �����尡 ������ ���̹Ƿ� ��õ�
	} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, // ������ ��� �ּ�
		LockFreeCheckCount, // ���ο� üũ��
		(LONG64)NewNode, // ���ο� _TopNode
		(LONG64*)&Top // ���� ������
	));

	InterlockedIncrement(&_LockFreeCount);
}

template<class DATA>
bool LockfreeStack<DATA>::Pop(DATA* Data)
{
	CheckNode PopNode; // pop�� ��� ������ ��Ƶ� ����
	Node* FreeNode; // Ǯ�� ��ȯ�� ���

	/*
		LockFreeCount�� ������� �ȿ� ���빰�� ���ٴ� ���̴ϱ�
		���ҽ�Ų���� �ٽ� �������� ������.
	*/
	if (InterlockedDecrement(&_LockFreeCount) < 0)
	{
		InterlockedIncrement(&_LockFreeCount);
		return false;
	}

	// CheckCount ����
	LONG64 LockFreeCheckCount = InterlockedIncrement64(&_LockFreeCheckCount);

	do
	{
		// ���� Stack�� TopNode�� ���������� ����
		PopNode.TopNode = _TopNode->TopNode;
		PopNode.NodeCheckValue = _TopNode->NodeCheckValue;

		// Pop�� ��� ���� (�޸� Ǯ ��ȯ��)
		FreeNode = PopNode.TopNode;

		// _TopNode�� (TopNode->NextNode, LockFreeCheckCount)�� ����
		// �����ϸ� �ٸ� �����尡 ������ ���̹Ƿ� ��õ�
	} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, // ������ ��� �ּ�
		LockFreeCheckCount, // ���ο� üũ��
		(LONG64)_TopNode->TopNode->NextNode, // ���ο� _TopNode
		(LONG64*)&PopNode // ���� ������
	));

	// Pop�� �����͸� ��ȯ
	*Data = PopNode.TopNode->Data;
	// ����� ��带 �޸� Ǯ�� ��ȯ
	_ObjectPool->Free(FreeNode);
	return true;
}
