#pragma once

#include"../Packet.h"
#include"../RingBuffer.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

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
