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
		wcout << L"1�� �̻��� ���̸� �����ϼ���" << endl;
		return false;
	}

	for (int i = 0; i < dummyClientCount; i++)
	{
		DummyClientSession* dummyClientSession = new DummyClientSession();
		_dummyClientList.push_back(dummyClientSession);
	}

	wcout << L"���� Ŭ���̾�Ʈ�� ������ " << dummyClientCount << L"�� ����" << endl;

	return true;
}

