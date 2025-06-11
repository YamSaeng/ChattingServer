#pragma once
#include"pch.h"
#include"../RingBuffer.h"
#include"../Packet.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct IOBlock
{
	LONG64 ioCount = 0;
	LONG64 isRelease = false;
};

struct DummyClientSession
{
	LONG dummyClientSessionId = 0;
	SOCKET clientSocket;

	SOCKADDR_IN serverAddr;

	RingBuffer recvRingBuffer;
	Packet* sendPacket = nullptr;	

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};		

	IOBlock* ioBlock = nullptr;
};