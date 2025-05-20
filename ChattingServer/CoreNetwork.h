#pragma once
#pragma comment(lib,"ws2_32")

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<process.h>
#include<sysinfoapi.h>
#include<iostream>

using namespace std;

class CoreNetwork
{
public:
	CoreNetwork();
	~CoreNetwork();
private:
	// IOCP �ڵ� 
	HANDLE _HCP;

	// ���� ����
	SOCKET _listenSocket;

	// ���� ����
	bool Start(const WCHAR* openIP, int port);

	// Accept ���� ������ �Լ�
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker ���� ������ �Լ�
	static unsigned __stdcall WorkerThreadProc(void* argument);
};