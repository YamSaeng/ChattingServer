#pragma once
#pragma comment(lib,"ws2_32")

#include"DummyClientSession.h"

class DummyClient
{
public:
	DummyClient();
	~DummyClient();	

	DummyClientSession* _dummyClientSession;
private:
	int _id;
	SOCKET _clientSocket;
	HANDLE _hIOCP;

	bool _connected;
	LONG _isSending;

public:		
	bool Connect(const wchar_t* ip, int port, int id, HANDLE hIOCP);
	void Disconnect(void);
	void SendRandomMessage(void);
	void RecvPost(void);
	void RecvComplete(DWORD transferred);
	void SendComplete(void);
};