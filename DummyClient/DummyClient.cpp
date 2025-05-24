#include "DummyClient.h"

DummyClient::DummyClient()
{
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		DWORD error = WSAGetLastError();
		wprintf(L"WSAStartup Error %d\n", error);
	}

	HANDLE hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThreadProc, this, 0, NULL);
	CloseHandle(hConnectThread);
}

DummyClient::~DummyClient()
{
}

unsigned __stdcall DummyClient::ConnectThreadProc(void* argument)
{
	return 0;
}
