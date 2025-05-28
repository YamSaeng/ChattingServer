#pragma once
#pragma comment(lib,"ws2_32")

#include"DummyClientSession.h"

class DummyClient
{
public:
	DummyClient();
	~DummyClient();	
private:
	HANDLE _dummyClientHCP;

	LONG _dummyClientSessionId;

	HANDLE _connetThreadWakeEvent;

	static unsigned __stdcall ConnectThreadProc(void* argument);

	static unsigned __stdcall WorkerThreadProc(void* argument);
public:
	int _duumyClientCount;
	list<DummyClientSession*> _dummyClientList;
	
	void DummyClientStart(int dummyClientCount);
};