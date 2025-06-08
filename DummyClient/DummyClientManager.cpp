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

void DummyClientManager::Start(int clientCount, const wchar_t* ip, int port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DWORD error = WSAGetLastError();
		cout << "DummyClientManager WSAStartup failed: " << error << endl;
		return;
	}

	// IOCP �ڵ� ����
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	_running = true;

	// ��Ŀ������ ����
	CreateWorkerThread();

	Sleep(1000);

	// 1�� ��ٷȴٰ� ���� Ŭ�� �����ϸ鼭 ���� �����ϰ� ����
	for (int i = 0; i < clientCount; i++)
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
	// �� ���μ��� *2 ��ŭ ��Ŀ������ ����
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

			bool result = GetQueuedCompletionStatus(instance->_hIOCP,
				&transferred, (PULONG_PTR)&completeDummyClient, (LPOVERLAPPED*)&completeOverlapped, INFINITE);

			if (completeOverlapped == nullptr)
			{
				DWORD GQCSError = WSAGetLastError();
				cout << "completeOverlapped NULL: " << GQCSError << endl;
				return -1;
			}

			if (completeOverlapped == &completeDummyClient->_dummyClientSession->recvOverlapped)
			{
				completeDummyClient->RecvComplete(transferred);
			}			
			else if (completeOverlapped == &completeDummyClient->_dummyClientSession->sendOverlapped)
			{
				completeDummyClient->SendComplete();
			}
		}
	}

	return 0;
}

unsigned __stdcall DummyClientManager::DummySendThreadProc(void* argument)
{
	return 0;
}