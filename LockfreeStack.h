#pragma once

#include "ObjectPool.h"

template <class DATA>
class LockfreeStack
{
private:
	struct Node
	{
		DATA Data;		// 저장할 데이터
		Node* NextNode; // 다음 노드를 가리키는 포인터
	};

	// ABA 문제 해결을 위한 구조체
	struct CheckNode
	{
		Node* TopNode; // 최상위 노드 포인터
		LONG64 NodeCheckValue; // ABA 문제 방지용 카운팅 변수
	};
	
	CheckNode* _TopNode; // Stack의 최상위 노드를 담아둠

	LONG64 _LockFreeCheckCount;	// ABA 방지용 카운팅 변수
	ObjectPool<Node>* _ObjectPool; // Node 풀
public:
	LockfreeStack();
	~LockfreeStack();

	LONG _LockFreeCount; // Stack에 있는 데이터 개수

	void Push(DATA Data); // Stack에 데이터 추가
	bool Pop(DATA* Data); // Stack에서 데이터 가져옴
};

template<class DATA>
LockfreeStack<DATA>::LockfreeStack()
{
	// 노드 메모리 풀 생성
	_ObjectPool = new ObjectPool<Node>();
	// 16바이트 정렬된 메모리로 할당 (128bit CAS를 위해)
	_TopNode = (CheckNode*)_aligned_malloc(sizeof(CheckNode), 16);

	// 초기 상태로 초기화
	_TopNode->TopNode = nullptr;
	_TopNode->NodeCheckValue = 0;

	// 카운터 값 초기화
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
	CheckNode Top; // 현재 Stack 상태를 읽어올 임시 변수
	Node* NewNode = _ObjectPool->Alloc(); // 풀에서 새 노드 할당
	NewNode->Data = Data; // 데이터 저장

	// CheckCount 증가
	LONG64 LockFreeCheckCount = InterlockedIncrement64(&_LockFreeCheckCount);

	do
	{
		// 현재 Stack의 TopNode를 지역변수에 저장
		Top.TopNode = _TopNode->TopNode;
		Top.NodeCheckValue = _TopNode->NodeCheckValue;

		// 새 노드가 최상위 노드를 가리키도록 설정
		NewNode->NextNode = _TopNode->TopNode;

		// _TopNode를 (NewNode, LockFreeCheckCount)로 변경
		// 실패하면 다른 스레드가 변경한 것이므로 재시도
	} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, // 변경할 대상 주소
		LockFreeCheckCount, // 새로운 체크값
		(LONG64)NewNode, // 새로운 _TopNode
		(LONG64*)&Top // 비교할 기존값
	));

	InterlockedIncrement(&_LockFreeCount);
}

template<class DATA>
bool LockfreeStack<DATA>::Pop(DATA* Data)
{
	CheckNode PopNode; // pop할 노드 정보를 담아둘 변수
	Node* FreeNode; // 풀로 반환할 노드

	/*
		LockFreeCount가 음수라면 안에 내용물이 없다는 것이니까
		감소시킨것을 다시 돌려놓고 나간다.
	*/
	if (InterlockedDecrement(&_LockFreeCount) < 0)
	{
		InterlockedIncrement(&_LockFreeCount);
		return false;
	}

	// CheckCount 증가
	LONG64 LockFreeCheckCount = InterlockedIncrement64(&_LockFreeCheckCount);

	do
	{
		// 현재 Stack의 TopNode를 지역변수에 저장
		PopNode.TopNode = _TopNode->TopNode;
		PopNode.NodeCheckValue = _TopNode->NodeCheckValue;

		// Pop할 노드 저장 (메모리 풀 반환용)
		FreeNode = PopNode.TopNode;

		// _TopNode를 (TopNode->NextNode, LockFreeCheckCount)로 변경
		// 실패하면 다른 스레드가 변경한 것이므로 재시도
	} while (!InterlockedCompareExchange128((volatile LONG64*)_TopNode, // 변경할 대상 주소
		LockFreeCheckCount, // 새로운 체크값
		(LONG64)_TopNode->TopNode->NextNode, // 새로운 _TopNode
		(LONG64*)&PopNode // 비교할 기존값
	));

	// Pop된 데이터를 반환
	*Data = PopNode.TopNode->Data;
	// 사용한 노드를 메모리 풀로 반환
	_ObjectPool->Free(FreeNode);
	return true;
}
