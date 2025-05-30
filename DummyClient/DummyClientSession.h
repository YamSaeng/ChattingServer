#pragma once
#include"pch.h"

#include"../Packet.h"
#include"../RingBuffer.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct DummyClientSession
{
	LONG dummyClientSessionId = 0;
	SOCKET clientSocket;

	SOCKADDR_IN serverAddr; // 접속할 서버 주소

	RingBuffer recvRingBuffer; // recvBuf
	queue<Packet> sendQueue; // sendQueue

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};