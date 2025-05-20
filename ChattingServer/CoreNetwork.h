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
	// IOCP 핸들 
	HANDLE _HCP;

	// 리슨 소켓
	SOCKET _listenSocket;

	// 서버 시작
	bool Start(const WCHAR* openIP, int port);

	// Accept 전용 스레드 함수
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker 전용 스레드 함수
	static unsigned __stdcall WorkerThreadProc(void* argument);
};