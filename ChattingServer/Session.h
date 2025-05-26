#pragma once

#include"RingBuffer.h"
#include"Packet.h"

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // Ŭ���̾�Ʈ ����
	
	SOCKADDR_IN clientAddr; // ������ ������ Ŭ�� �ּ�

	RingBuffer recvRingBuffer; // recvBuf
	queue<Packet> sendQueue; // sendQueue

	OVERLAPPED recvOverlapped = {}; // WSARecv ����
	OVERLAPPED sendOverlapped = {};	// WSASend ����
};
