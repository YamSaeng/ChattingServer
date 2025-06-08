#pragma once
#include"pch.h"
#include"../Packet.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct DummyClientSession
{
	LONG dummyClientSessionId = 0;
	SOCKET clientSocket;

	SOCKADDR_IN serverAddr;

	char recvBuffer[1024] = {};	
	Packet* sendPacket = nullptr;	

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};