#pragma once
#include"pch.h"

struct DummyClientSession
{
	LONG dummyClientSessionId = 0;
	SOCKET clientSocket;

	SOCKADDR_IN serverAddr;

	char recvBuffer[1024] = {};
	char sendBuffer[1024] = {};

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};