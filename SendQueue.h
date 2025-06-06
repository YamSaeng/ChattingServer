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

	// packet �ֱ�
	void Push(Packet* packet);

	// packet ����
	Packet* Pop();

	// packet�� �ִ��� Ȯ��
	bool IsEmpty();

	// ���� queue û��
	void Clear();

	// queue�� �ִ� packet�� ���� ��ȯ
	size_t Size();
};