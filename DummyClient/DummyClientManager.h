#pragma once
#include"pch.h"

class DummyClient;

class DummyClientManager
{
public:
	DummyClientManager();
	~DummyClientManager();	
private:
	void CreateWorkerThread(void);

	static unsigned __stdcall DummyWorkerThreadProc(void* argument);
	static unsigned __stdcall DummySendThreadProc(void* argument);	

	HANDLE _hSendThread;
	HANDLE _hIOCP;

	bool _running;
	int _workerThreadCount;

	vector<DummyClient*> _clients;
	vector<HANDLE> _hWorkerThreads;	
public:
	void Start(int clientCount, const wchar_t* ip, int port);
	void Stop(void);
	
	LONG _sendPacketTPS;
};