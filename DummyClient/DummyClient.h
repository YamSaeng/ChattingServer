#pragma once
#pragma comment(lib,"ws2_32")

#include"DummyClientSession.h"

class DummyClient
{
public:
	DummyClient();
	~DummyClient();	
public:
	list<DummyClientSession*> _dummyClientList;

	bool DummyClientCountSet(int dummyClientCount);
};