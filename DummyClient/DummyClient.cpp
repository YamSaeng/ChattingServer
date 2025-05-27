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

	_connetThreadWakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

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
	DummyClient* Instance = (DummyClient*)argument;

	if (Instance != nullptr)
	{
		SOCKADDR_IN serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);
		serverAddr.sin_port = htons(8888);

		while (!Instance->_connetThreadWakeEvent)
		{
			WaitForSingleObject(Instance->_connetThreadWakeEvent, INFINITE);

			while (1)
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

				dummyClientSession->dummyClientSessionId = ++Instance->_dummyClientSessionId;
				dummyClientSession->serverAddr = serverAddr;
			}
		}
	}

	return 0;
}

unsigned __stdcall DummyClient::WorkerThreadProc(void* argument)
{
	return 0;
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

