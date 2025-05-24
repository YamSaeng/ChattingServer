#pragma once
#pragma comment(lib,"ws2_32")

#include<iostream>
#include<Windows.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<process.h>

class DummyClient
{
public:
	DummyClient();
	~DummyClient();
private:
	static unsigned __stdcall ConnectThreadProc(void* argument);
};