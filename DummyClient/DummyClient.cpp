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

	_dummyClientSessionId = 0;

	// ���ȣ, �ڵ� ���� ���·� �̺�Ʈ ����
	_connetThreadWakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	_duumyClientCount = 0;

	_dummyClientHCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_dummyClientHCP == NULL)
	{
		DWORD error = WSAGetLastError();
		std::cout << "CreateIoCompletionPort failed : " << error << std::endl;
	}

	SYSTEM_INFO SI;
	GetSystemInfo(&SI);

	HANDLE hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThreadProc, this, 0, NULL);
	CloseHandle(hConnectThread);

	for (int i = 0; i < SI.dwNumberOfProcessors; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, this, 0, NULL);
		CloseHandle(hWorkerThread);
	}
}

DummyClient::~DummyClient()
{
}

unsigned __stdcall DummyClient::ConnectThreadProc(void* argument)
{
	DummyClient* instance = (DummyClient*)argument;

	if (instance != nullptr)
	{
		SOCKADDR_IN serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);
		serverAddr.sin_port = htons(8888);

		WaitForSingleObject(instance->_connetThreadWakeEvent, INFINITE);

		for (int i = 0; i < instance->_duumyClientCount; i++)
		{
			DummyClientSession* dummyClientSession = new DummyClientSession();
			dummyClientSession->clientSocket = socket(AF_INET, SOCK_STREAM, 0);

			int connectRet = connect(dummyClientSession->clientSocket, (SOCKADDR*)&serverAddr, sizeof(dummyClientSession->serverAddr));
			if (connectRet == SOCKET_ERROR)
			{
				DWORD connectError = WSAGetLastError();
				wcout << L"connect Error %d \n" << endl;
				break;
			}

			dummyClientSession->dummyClientSessionId = ++instance->_dummyClientSessionId;
			dummyClientSession->serverAddr = serverAddr;

			instance->_dummyClientList.push_back(dummyClientSession);
		}
	}

	return 0;
}

unsigned __stdcall DummyClient::WorkerThreadProc(void* argument)
{
	return 0;
}

void DummyClient::DummyClientStart(int dummyClientCount)
{
	if (dummyClientCount <= 0)
	{
		wcout << L"1�� �̻��� ���̸� �����ϼ���" << endl;
		return;
	}

	_duumyClientCount = dummyClientCount;	

	wcout << L"���� Ŭ���̾�Ʈ ���� ���� ���� [" << _dummyClientList.size() << L"] ��" << endl;

	SetEvent(_connetThreadWakeEvent);
}
