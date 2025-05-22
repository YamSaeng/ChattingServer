#pragma once
#pragma comment(lib,"ws2_32")

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<process.h>
#include<sysinfoapi.h>
#include<iostream>
#include<list>

#include"Session.h"

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

	// 1�� ���� ���� ���� ����
	INT32 _acceptTPS;
	// ���� ���� �� ����
	INT32 _acceptTotal;

	// ������ ������ sessionId;
	INT64 _sessionId;

	// �������� ������ session ����Ʈ
	list<Session*> _sessions;

	// ���� ����
	bool Start(const WCHAR* openIP, int port);

	// Accept ���� ������ �Լ�
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker ���� ������ �Լ�
	static unsigned __stdcall WorkerThreadProc(void* argument);

	// accept ȣ�� �� ������ Ŭ�� ������� ȣ���ϴ� �Լ�
	virtual void OnClientJoin(Session* newSession) = 0;
};