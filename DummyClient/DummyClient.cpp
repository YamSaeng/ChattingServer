#include"pch.h"
#include"DummyClient.h"
#include"DummyClientSession.h"

DummyClient::DummyClient()
{
	WSADATA WSA;
	if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0)
	{
		DWORD error = WSAGetLastError();
		wprintf(L"WSAStartup Error %d\n", error);
	}
}

DummyClient::~DummyClient()
{
}

bool DummyClient::DummyClientCountSet(int dummyClientCount)
{
	if (dummyClientCount <= 0)
	{
		wcout << L"1개 이상의 더미를 생성하세요" << endl;
		return false;
	}

	for (int i = 0; i < dummyClientCount; i++)
	{
		DummyClientSession* dummyClientSession = new DummyClientSession();
		_dummyClientList.push_back(dummyClientSession);
	}

	wcout << L"더미 클라이언트의 개수를 " << dummyClientCount << L"로 설정" << endl;

	return true;
}

