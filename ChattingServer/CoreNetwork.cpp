#include"CoreNetwork.h"

CoreNetwork::CoreNetwork()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DWORD error = WSAGetLastError();
		std::cout << "WSAStartup failed : " << error << std::endl;
		return;
	}

	_acceptTPS = 0;
	_acceptTotal = 0;

	_sessionId = 0;	
}

CoreNetwork::~CoreNetwork()
{
}

bool CoreNetwork::Start(const WCHAR* openIP, int port)
{
	// ���� ���� ����
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed" << std::endl;
		return false;
	}

	// ���ε�
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	InetPton(AF_INET, openIP, &serverAddr.sin_addr);

	int bindResult = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN));
	if (bindResult == SOCKET_ERROR)
	{
		std::cout << "bind failed" << std::endl;
		return false;
	}

	// ����
	int listenResult = listen(_listenSocket, SOMAXCONN);
	if (listenResult == SOCKET_ERROR)
	{
		std::cout << "listen failed" << std::endl;
		return false;
	}

	// IOCP Port�� �����Ѵ�.
	_HCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_HCP == NULL)
	{
		std::cout << "CreateIoCompletionPort failed" << std::endl;
		return false;
	}

	// accept ������ ����
	HANDLE hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadProc, this, 0, NULL);
	CloseHandle(hAcceptThread);

	// IOCP ��Ŀ ������ ����
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);

	for (int i = 0; i < (int)SI.dwNumberOfProcessors; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, this, 0, NULL);
		CloseHandle(hWorkerThread);
	}
}

unsigned __stdcall CoreNetwork::AcceptThreadProc(void* argument)
{
	CoreNetwork* instance = (CoreNetwork*)argument;

	if (instance != nullptr)
	{
		for (;;)
		{
			SOCKADDR_IN clientAddr;
			int addrLen = sizeof(clientAddr);

			// Ŭ�� ���� ��� 
			SOCKET clientSock = accept(instance->_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSock == INVALID_SOCKET)
			{
				DWORD error = WSAGetLastError();
				std::cout << "accept failed : " << error << std::endl;
				break;
			}

			// ���� ���� �� ���� ����
			instance->_acceptTotal++;
			instance->_acceptTPS++;

			// ���� �Ҵ�
			Session* newSession = new Session();			

			newSession->sessionId = ++instance->_sessionId;
			newSession->clientAddr = clientAddr;
			newSession->clientSocket = clientSock;

			// IOCP�� ���
			CreateIoCompletionPort((HANDLE)newSession->clientSocket, instance->_HCP, (ULONG_PTR)newSession, 0);

			instance->OnClientJoin(newSession);

			// sessions�� ����
			instance->_sessions.push_back(newSession);
		}
	}

	return 0;
}

unsigned __stdcall CoreNetwork::WorkerThreadProc(void* argument)
{
	return 0;
}
