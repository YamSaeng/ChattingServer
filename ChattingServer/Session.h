#pragma once

#include"../Packet.h"
#include"../RingBuffer.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct IOBlock
{
	// IO 작업 횟수를 기록해둘 변수 
	// Session을 사용하고 있는지에 대한 여부
	LONG64 IOCount = 0;
	
	// Release를 했는지 안했는지에 대한 여부
	LONG64 IsRelease = false;
};

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // 클라이언트 소켓
	
	SOCKADDR_IN clientAddr; // 서버에 접속한 클라 주소

	RingBuffer recvRingBuffer; // recvBuf
	queue<Packet*> sendQueue; // sendQueue

	OVERLAPPED recvOverlapped = {}; // WSARecv 통지
	OVERLAPPED sendOverlapped = {};	// WSASend 통지

	IOBlock* IOBlock = nullptr;
};
