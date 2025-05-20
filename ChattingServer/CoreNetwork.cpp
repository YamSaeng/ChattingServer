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
}

CoreNetwork::~CoreNetwork()
{
}

bool CoreNetwork::Start(const WCHAR* openIP, int port)
{
	// 府郊 家南 积己
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed" << std::endl;
		return false;
	}

	// 官牢靛
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

	// 府郊
	int listenResult = listen(_listenSocket, SOMAXCONN);
	if (listenResult == SOCKET_ERROR)
	{
		std::cout << "listen failed" << std::endl;
		return false;
	}

	// IOCP Port甫 积己茄促.
	_HCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_HCP == NULL)
	{
		std::cout << "CreateIoCompletionPort failed" << std::endl;
		return false;
	}

	// accept 胶饭靛 积己
	HANDLE hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadProc, this, 0, NULL);
	CloseHandle(hAcceptThread);

	// IOCP 况目 胶饭靛 积己
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);
	
	for(int i=0;i<(int)SI.dwNumberOfProcessors;i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, this, 0, NULL);
		CloseHandle(hWorkerThread);
	}
}

unsigned __stdcall CoreNetwork::AcceptThreadProc(void* argument)
{
	return 0;
}

unsigned __stdcall CoreNetwork::WorkerThreadProc(void* argument)
{
	return 0;
}
