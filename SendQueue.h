#pragma once
#include"pch.h"
#include"Packet.h"

#ifdef SEND_QUEUE
#define SEND_QUEUE_DLL __declspec(dllexport)
#else
#define SEND_QUEUE_DLL __declspec(dllimport)
#endif

class SEND_QUEUE_DLL SendQueue
{
private:
	queue<Packet*> queue;
	CRITICAL_SECTION cs;
public:
	SendQueue();
	~SendQueue();

	// packet 넣기
	void Push(Packet* packet);

	// packet 빼기
	Packet* Pop();

	// packet이 있는지 확인
	bool IsEmpty();

	// 내부 queue 청소
	void Clear();

	// queue에 있는 packet의 개수 반환
	size_t Size();
};