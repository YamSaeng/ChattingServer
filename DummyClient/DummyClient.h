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
	list<DummyClientSession*> _dummyClientList;

	bool DummyClientCountSet(int dummyClientCount);
	void DummyClientStart(void);
};