#include"pch.h"
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

			newSession->IOBlock = new IOBlock();			

			// IOCP�� ���
			CreateIoCompletionPort((HANDLE)newSession->clientSocket, instance->_HCP, (ULONG_PTR)newSession, 0);

			newSession->IOBlock->IOCount++;
			newSession->IOBlock->IsRelease = 0;

			instance->OnClientJoin(newSession);
			instance->RecvPost(newSession);

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

void CoreNetwork::RecvPost(Session* recvSession, bool isAcceptRecvPost)
{
	int recvBuffCount = 0;
	WSABUF recvBuf[2];

	// recvRingBuffer�� �ѹ��� ���� �� �ִ� ũ�⸦ �д´�.
	int directEnqueueSize = recvSession->recvRingBuffer.GetDirectEnqueueSize();
	// ���� �ִ� recvRingBuffer�� ũ�⸦ �д´�.
	int recvRingBuffFreeSize = recvSession->recvRingBuffer.GetFreeSize();

	// ���� ���� �ִ� recvRingBuffer�� ũ�Ⱑ �ѹ��� ���� �� �ִ� ũ�⺸�� ũ�ٸ�
	// recvRingBuffer�� 2���� �������� ������ �ִ� ���� Ȯ���� �� �ִ�.
	if (recvRingBuffFreeSize > directEnqueueSize)
	{
		recvBuffCount = 2;
		recvBuf[0].buf = recvSession->recvRingBuffer.GetRearBufferPtr();
		recvBuf[0].len = directEnqueueSize;

		recvBuf[1].buf = recvSession->recvRingBuffer.GetBufferPtr();
		recvBuf[1].len = recvRingBuffFreeSize - directEnqueueSize;
	}
	else
	{
		recvBuffCount = 1;
		recvBuf[0].buf = recvSession->recvRingBuffer.GetRearBufferPtr();
		recvBuf[0].len = directEnqueueSize;
	}

	// WSARecv�� �ɱ� ���� �ѹ� û�����ش�.
	memset(&recvSession->recvOverlapped, 0, sizeof(OVERLAPPED));
		
	if (isAcceptRecvPost == false)
	{
		InterlockedIncrement64(&recvSession->IOBlock->IOCount);
	}	

	DWORD flags = 0;

	int WSARecvRetval = WSARecv(recvSession->clientSocket, recvBuf, recvBuffCount, NULL, &flags, (LPWSAOVERLAPPED)&recvSession->recvOverlapped, NULL);
	if (WSARecvRetval == SOCKET_ERROR)
	{
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (InterlockedDecrement64(&recvSession->IOBlock->IOCount) == 0)
			{
				ReleaseSession(recvSession);
			}
		}
	}
}

void CoreNetwork::ReleaseSession(Session* releaseSession)
{
	IOBlock compareBlock;
	compareBlock.IOCount = 0;
	compareBlock.IsRelease = 0;

	// release Session�� ���ؼ� ���� CAS�� ����
	// IOCount�� 0 ����( ��������� �ƴ��� Ȯ�� )��
	// IsRelease�� true ����( Release�� �� ���� ���� Ȯ�� )�� Ȯ���Ѵ�.
	if (!InterlockedCompareExchange128((LONG64*)releaseSession->IOBlock, (LONG64)true, (LONG64)0, (LONG64*)&compareBlock))
	{
		return;
	}

	releaseSession->recvRingBuffer.ClearBuffer();

	while (releaseSession->sendQueue.size() > 0)
	{
		Packet* deletePacket = releaseSession->sendQueue.front();
		if (deletePacket == nullptr)
		{
			continue;
		}

		delete deletePacket;
	}
}

Session* CoreNetwork::FindSession(__int64 sessionId)
{
	int sessionIndex = GET_SESSION_INDEX(sessionId);

	Session* findSession = _sessions[sessionIndex];
	if (findSession == nullptr)
	{
		return nullptr;
	}

	// ���� session�� release �۾� �����̶�� ������.
	if (findSession->IOBlock->IsRelease == 1)
	{
		return nullptr;
	}	

	// IOCount�� 1 ���������� �� �� ���� 1���� Ȯ���Ѵ�.
	// 1 �̶�� �ش� session�� release ���̰ų� �������� �� �� �ִ�.
	if (InterlockedIncrement64(&findSession->IOBlock->IOCount) == 1)
	{
		// ���⼭ IOCount�� 1 ���ҽ����� �� 0�̶�� release�� �����Ѵ�.
		// �ٸ� ������ IOCount�� �����ϰ�, ���� ReleaseSession�� ȣ���ϱ� ���� �� �����ϱ�.
		if (InterlockedDecrement64(&findSession->IOBlock->IOCount) == 0)
		{
			ReleaseSession(findSession);
		}

		// �ƴ϶�� nullptr�� ��ȯ�� release ������� �˷��ش�.
		return nullptr;
	}

	return nullptr;
}

void CoreNetwork::ReturnSession(Session* session)
{
	if (InterlockedDecrement64(&session->IOBlock->IOCount) == 0)
	{
		ReleaseSession(session);
	}
}
