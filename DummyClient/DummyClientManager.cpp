#include"pch.h"
#include"DummyClientManager.h"
#include"DummyClient.h"

DummyClientManager::DummyClientManager()
{

}

DummyClientManager::~DummyClientManager()
{
	WSACleanup();
}

void DummyClientManager::Start(int clientCount, bool isDisconnect, bool isReconnectTry, char probilityDisconnect, const wchar_t* ip, int port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DWORD error = WSAGetLastError();
		cout << "DummyClientManager WSAStartup failed: " << error << endl;
		return;
	}

	// IOCP 핸들 생성
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	_running = true;

	_clientCount = clientCount;
	_isDisconnect = isDisconnect;
	_isReconnectTry = isReconnectTry;
	_probilityDisconnect = probilityDisconnect;

	// 워커쓰레드 생성
	CreateWorkerThread();

	Sleep(1000);

	// 1초 기다렸다가 더미 클라 생성하면서 서버 접속하고 저장
	for (int i = 0; i < _clientCount; i++)
	{
		DummyClient* dummyClient = new DummyClient();
		dummyClient->Connect(ip, port, i, _hIOCP);
		_clients.push_back(dummyClient);
	}
}

void DummyClientManager::Stop(void)
{
	_running = false;
}

void DummyClientManager::CreateWorkerThread(void)
{
	// 논리 프로세서 *2 만큼 워커스레드 생성
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	_workerThreadCount = systemInfo.dwNumberOfProcessors * 2;

	for (int i = 0; i < _workerThreadCount; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, DummyWorkerThreadProc, this, 0, nullptr);
		if (hThread)
		{
			_hWorkerThreads.push_back(hThread);
		}
	}

	_hSendThread = (HANDLE)_beginthreadex(NULL, 0, DummySendThreadProc, this, 0, nullptr);
}

unsigned __stdcall DummyClientManager::DummyWorkerThreadProc(void* argument)
{
	DummyClientManager* instance = (DummyClientManager*)argument;

	if (instance != nullptr)
	{
		DummyClient* completeDummyClient = nullptr;
		while (instance->_running)
		{
			DWORD transferred = 0;
			OVERLAPPED* completeOverlapped = nullptr;

			do
			{
				bool result = GetQueuedCompletionStatus(instance->_hIOCP,
					&transferred, (PULONG_PTR)&completeDummyClient, (LPOVERLAPPED*)&completeOverlapped, INFINITE);

				if (completeOverlapped == nullptr)
				{
					DWORD GQCSError = WSAGetLastError();
					cout << "completeOverlapped NULL: " << GQCSError << endl;
					return -1;
				}

				if (transferred == 0)
				{
					break;
				}

				if (completeOverlapped == &completeDummyClient->_dummyClientSession->recvOverlapped)
				{
					completeDummyClient->RecvComplete(transferred);
				}
				else if (completeOverlapped == &completeDummyClient->_dummyClientSession->sendOverlapped)
				{
					completeDummyClient->SendComplete();
				}
			} while (0);

			if (InterlockedDecrement64(&completeDummyClient->_dummyClientSession->ioBlock->ioCount) == 0)
			{
				completeDummyClient->ReleaseDummyClient();
			}
		}
	}

	return 0;
}

unsigned __stdcall DummyClientManager::DummySendThreadProc(void* argument)
{
	Sleep(2000);

	DummyClientManager* instance = (DummyClientManager*)argument;
	if (instance != nullptr)
	{
		srand((unsigned int)time(NULL));

		while (instance->_running)
		{
			for (auto client : instance->_clients)
			{
				if (client->_connected)
				{
					int randomValue = rand() % 100;

					if (randomValue < 100 - instance->_probilityDisconnect)
					{
						client->SendRandomMessage();
						instance->_sendPacketTPS++;
					}
					else
					{
						client->Disconnect();
					}
				}
				else
				{
					if (instance->_isReconnectTry && client->ReconnectTimeCheck())
					{
						client->ReconnectTry();
					}
				}
			}

			Sleep(500);
		}
	}

	return 0;
}