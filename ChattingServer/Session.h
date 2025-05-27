#pragma once

#include"../Packet.h"
#include"../RingBuffer.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // 클라이언트 소켓
	
	SOCKADDR_IN clientAddr; // 서버에 접속한 클라 주소

	RingBuffer recvRingBuffer; // recvBuf
	queue<Packet> sendQueue; // sendQueue

	OVERLAPPED recvOverlapped = {}; // WSARecv 통지
	OVERLAPPED sendOverlapped = {};	// WSASend 통지
};
