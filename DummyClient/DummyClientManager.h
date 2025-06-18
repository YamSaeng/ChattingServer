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

	int _clientCount;
	bool _isDisconnect;
	bool _isReconnectTry;
	int _probilityDisconnect;

	bool _running;
	int _workerThreadCount;

	vector<DummyClient*> _clients;
	vector<HANDLE> _hWorkerThreads;
public:
	void Start(int clientCount, bool isDisconnect, bool isReconnectTry, char probilityDisconnect, const wchar_t* ip, int port);
	void Stop(void);

	LONG _sendPacketTPS;
};