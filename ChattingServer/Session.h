#pragma once

#include"../Packet.h"
#include"../RingBuffer.h"
#include"../SendQueue.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct IOBlock
{
	// IO 작업 횟수를 기록해둘 변수 
	// Session을 사용하고 있는지에 대한 여부
	LONG64 ioCount = 0;
	
	// Release를 했는지 안했는지에 대한 여부
	LONG64 IsRelease = false;
};

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // 클라이언트 소켓
	
	SOCKADDR_IN clientAddr; // 서버에 접속한 클라 주소

	RingBuffer recvRingBuffer; // recvBuf
	SendQueue sendQueue; // sendQueue	

	OVERLAPPED recvOverlapped = {}; // WSARecv 통지
	OVERLAPPED sendOverlapped = {};	// WSASend 통지

	IOBlock* ioBlock = nullptr; // Session이 사용중인지, 해제되었는지 확인
	
	LONG isSend; // 세션에 대해 WSASend 작업을 하고 있는지 안하고 있는지 여부

	SendQueue sendPacket; // 전송할 패킷들을 담아둠
	LONG sendPacketCount; // 전송한 패킷의 개수를 기록
};
