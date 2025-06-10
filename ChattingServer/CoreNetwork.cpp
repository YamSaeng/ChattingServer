#include "CoreNetwork.h"
#include"pch.h"
#include"CoreNetwork.h"

CoreNetwork::CoreNetwork()
{
	_acceptTPS = 0;
	_acceptTotal = 0;

	_sessionId = 0;

	_listenSocket = NULL;
	_HCP = NULL;

	_hAcceptThread = NULL;
	
	_recvPacketTPS = 0;
	_sendPacketTPS = 0;
}

CoreNetwork::~CoreNetwork()
{
}

bool CoreNetwork::Start(const WCHAR* openIP, int port)
{
	wcout << L"ä�� ���� ����" << endl;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DWORD error = WSAGetLastError();
		std::cout << "WSAStartup failed : " << error << std::endl;
		return false;
	}

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
	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadProc, this, 0, NULL);	

	// IOCP ��Ŀ ������ ����
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);

	for (int i = 0; i < (int)SI.dwNumberOfProcessors; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, this, 0, NULL);		
		_hWorkerThreadHandles.push_back(hWorkerThread);
	}
}

void CoreNetwork::Stop()
{
	wcout << L"ä�� ���� ����" << endl;
	
	// session ���� ����
	for (Session* session : _sessions)
	{
		if (session != nullptr)
		{
			Disconnect(session->sessionId);
		}
	}

	// listen ���� �ݱ�
	closesocket(_listenSocket);
	
	WSACleanup();
	
	// session �Ҵ� ����
	for (Session* session : _sessions)
	{
		closesocket(session->clientSocket);
		delete session->ioBlock;
		delete session;
	}

	// session �迭 ����
	_sessions.clear();

	// accept ������ ����
	CloseHandle(_hAcceptThread);
	_hAcceptThread = NULL;

	for (HANDLE workerThread : _hWorkerThreadHandles)
	{
		CloseHandle(workerThread);
	}

	_hWorkerThreadHandles.clear();

	// IOCP �ڵ� �ݱ�
	CloseHandle(_HCP);
	_HCP = NULL;
		
	_acceptTPS = 0;
	_acceptTotal = 0;
	_sessionId = 0;
	_listenSocket = NULL;	
		
	wcout << L"ä�� ���� ���� �Ϸ�" << endl;
}

void CoreNetwork::SendPacket(__int64 sessionId, Packet* packet)
{	
	Session* sendSession = FindSession(sessionId);
	if (sendSession == nullptr)
	{
		return;
	}

	packet->Encode();

	// ��Ŷ ť��
	sendSession->sendQueue.Push(packet);

	// WSASend ���
	SendPost(sendSession);

	// FindSession���� ���������� IOCount�� �ٿ���
	ReturnSession(sendSession);
}

void CoreNetwork::Disconnect(__int64 sessionId)
{
	// ���� ������ session�� ã��
	Session* disconnectSession = FindSession(sessionId);	
	if (disconnectSession == nullptr)
	{		
		return;
	}

	if (disconnectSession->ioBlock->ioCount == 0)
	{
		ReleaseSession(disconnectSession);
	}
	else
	{
		CancelIoEx((HANDLE)disconnectSession->clientSocket, NULL);
	}

	ReturnSession(disconnectSession);
}

int CoreNetwork::SessionCount()
{
	return (int)_sessions.size();
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

			newSession->sessionId = instance->_sessionId++;
			newSession->clientAddr = clientAddr;
			newSession->clientSocket = clientSock;

			newSession->ioBlock = new IOBlock();			

			// IOCP�� ���
			CreateIoCompletionPort((HANDLE)newSession->clientSocket, instance->_HCP, (ULONG_PTR)newSession, 0);

			newSession->ioBlock->ioCount++;
			newSession->ioBlock->IsRelease = 0;

			instance->OnClientJoin(newSession);
			instance->RecvPost(newSession, true);

			// sessions�� ����
			instance->_sessions.push_back(newSession);
		}
	}

	return 0;
}

unsigned __stdcall CoreNetwork::WorkerThreadProc(void* argument)
{
	CoreNetwork* instance = (CoreNetwork*)argument;

	if (instance != nullptr)
	{
		Session* completeSession = nullptr;
		while (1)
		{
			DWORD transferred = 0;
			OVERLAPPED* myOverlapped = nullptr;
			int completeRet;
			DWORD GQCSError;

			do
			{
				completeRet = GetQueuedCompletionStatus(instance->_HCP, &transferred,
					(PULONG_PTR)&completeSession, (LPOVERLAPPED*)&myOverlapped, INFINITE);
				// myOverlapped�� nullptr�̶�� GetQueuedCompletionStatus�� ������ ���� return ó���Ѵ�.
				if (myOverlapped == nullptr)
				{
					GQCSError = WSAGetLastError();
					cout << "MyOverlapped NULL " << GQCSError << endl;
					return -1;
				}

				// transferred�� 0�̶�� fin ��Ŷ�� �������̹Ƿ� ����ó���Ѵ�.
				if (transferred == 0)
				{
					break;
				}

				// ���޹��� overlapped�� recvOverlapped��� recv �Ϸ� ó���� �����Ѵ�.
				if (myOverlapped == &completeSession->recvOverlapped)
				{
					instance->RecvComplete(completeSession, transferred);
				}
				// ���޹��� overlapped�� sendOverlapped��� send �Ϸ� ó���� �����Ѵ�.
				else if (myOverlapped == &completeSession->sendOverlapped)
				{
					instance->SendComplete(completeSession);
				}
			} while (0);

			// IO ó���� �ϳ� �������Ƿ� IOCount�� ���ҽ�Ų��.
			if (InterlockedDecrement64(&completeSession->ioBlock->ioCount) == 0)
			{
				instance->ReleaseSession(completeSession);
			}
		}
	}

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
		InterlockedIncrement64(&recvSession->ioBlock->ioCount);
	}	

	DWORD flags = 0;

	int WSARecvRetval = WSARecv(recvSession->clientSocket, recvBuf, recvBuffCount, NULL, &flags, (LPWSAOVERLAPPED)&recvSession->recvOverlapped, NULL);
	if (WSARecvRetval == SOCKET_ERROR)
	{
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (InterlockedDecrement64(&recvSession->ioBlock->ioCount) == 0)
			{
				ReleaseSession(recvSession);
			}
		}
	}
}

void CoreNetwork::SendPost(Session* sendSession)
{
	int sendUseSize;
	int sendCount = 0;
	WSABUF sendBuf[500];

	do
	{
		// Session�� isSend�� 1�� �ٲٸ鼭 �����Ѵ�.
		// �ٸ� ��Ŀ �����尡 WSASend�� ���� �ʵ��� ���´�.
		if (InterlockedExchange(&sendSession->isSend, 1) == 0)
		{
			sendUseSize = sendSession->sendQueue.Size();

			/*
				�������� ���������� isSend�� 1�̶� �ٸ� �����忡�� �̹� WSASend ���� �� �ִ�.
				���� sendQueue�� ��� �ִٰ� �Ǵ��ϰ� isSend�� 0���� �ٲٴ� ����
				�ٸ� �����尡 ��Ŷ�� �ְ� SendPost�� ȣ���� �� �ִ�.
				������ isSend�� ������ 1�̹Ƿ� �� ������� WSASend�� �õ����� ���ϰ� ����������.
				�ᱹ ���� �����嵵 �׳� ���������� ���� ���� ��Ŷ�� ���۵��� ���� ä�� ���� �ȴ�.
				���� isSend�� 0���� �ٲ� ����, ť�� �����Ͱ� ���Դ����� �ѹ� �� Ȯ���ϰ�,
				�����Ѵٸ� �ٽ� SendPost�� �ݺ��Ͽ� WSASend�� �õ��Ѵ�.

				ex) �Ϲ����� ��Ȳ
				A Thread ���� isSend = 1�� �����ϰ� ����
				sendQueue.size() == 0 Ȯ�� ���� ����				

				A, B Thread 2��
				A Thread ���� isSend = 1�� �����ϰ� ����
				sendQueue.size() == 0 Ȯ�� isSend = 0 ���� �ٲٱ� ����
				B Thread�� ��Ŷ�� ť���ϰ� ���� isSend�� 1�̶� �׳� ����

				B Thread���� ��Ŷ�� �־��µ� isSend�� 1�̶� �׳� ������
				A Thread������ sendQueue.size()�� 0�� ���� ������ isSend�� 0���� �ٲٰ� �׳� ����
				
				�ᱹ ť�� ��Ŷ�� ���� �ִµ� WSASend�� ���� �ʱ� ������ 

				A Thread���� sendQue�� �ٽ��ѹ� Ȯ���ؼ� SendPost�� �ٽ� �����Ų��.
			*/
			if (sendUseSize == 0)
			{
				InterlockedExchange(&sendSession->isSend, 0);

				if (!sendSession->sendQueue.IsEmpty())
				{
					continue;
				}
				else
				{
					return;
				}
			}			
		}
		else
		{
			return;
		}
	} while (0);	

	// �������� ��Ŷ�� ������ŭ sendQueue���� ��Ŷ�� ������ WSABUF�� �ִ´�.
	for (int i = 0; i < sendUseSize; i++)
	{
		if (sendSession->sendQueue.Size() == 0)
		{
			break;
		}

		InterlockedIncrement(&_sendPacketTPS);

		Packet* packet = sendSession->sendQueue.Pop();		
		
		sendBuf[i].buf = packet->GetHeaderBufferPtr();
		sendBuf[i].len = packet->GetUseBufferSize();

		sendSession->sendPacket.Push(packet);		
	}

	// WSAsend�� �ɱ� ���� �ѹ� û�����ش�.
	memset(&sendSession->sendOverlapped, 0, sizeof(OVERLAPPED));

	// IOCount�� 1 �������� �ش� session�� release ����� ���� �ʵ��� �Ѵ�.
	InterlockedIncrement64(&sendSession->ioBlock->ioCount);
		
	int WSASendRetval = WSASend(sendSession->clientSocket, sendBuf, sendUseSize, NULL, 0, (LPWSAOVERLAPPED)&sendSession->sendOverlapped, NULL);
	if (WSASendRetval == SOCKET_ERROR)
	{
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING)
		{
			if (InterlockedDecrement64(&sendSession->ioBlock->ioCount) == 0)
			{
				ReleaseSession(sendSession);
			}
		}
	}
}

void CoreNetwork::ReleaseSession(Session* releaseSession)
{
	IOBlock compareBlock;
	compareBlock.ioCount = 0;
	compareBlock.IsRelease = 0;

	// release Session�� ���ؼ� ���� CAS�� ����
	// IOCount�� 0 ����( ��������� �ƴ��� Ȯ�� )��
	// IsRelease�� true ����( Release�� �� ���� ���� Ȯ�� )�� Ȯ���Ѵ�.
	if (!InterlockedCompareExchange128((LONG64*)releaseSession->ioBlock, (LONG64)true, (LONG64)0, (LONG64*)&compareBlock))
	{
		return;
	}

	// recvBuf û��
	releaseSession->recvRingBuffer.ClearBuffer();

	// sendPacket�� ȣ���ؼ� ��Ŷ�� enqueue�� �ϰ� ���� ���ɼ��� �ֱ� ������
	// �� �κп��� sendQueue�� ũ�⸦ Ȯ���ؼ� ������ ���������� �����Ѵ�.
	while (releaseSession->sendQueue.Size() > 0)
	{
		Packet* deletePacket = releaseSession->sendQueue.Pop();
		
		if (deletePacket == nullptr)
		{
			continue;
		}

		delete deletePacket;
	}

	closesocket(releaseSession->clientSocket);
	delete releaseSession->ioBlock;

	_sessions.erase(remove_if(_sessions.begin(), _sessions.end(), [releaseSession](Session* eraseSession) {return eraseSession->sessionId == releaseSession->sessionId; }), _sessions.end());
}

void CoreNetwork::RecvComplete(Session* recvCompleteSesion, const DWORD& transferred)
{
	int loopCount = 0;
	const int MAX_PACKET_LOOP = 64;
	recvCompleteSesion->recvRingBuffer.MoveRear(transferred);

	Packet::EncodeHeader encodeHeader;
	Packet* packet = new Packet();

	while (loopCount++ < MAX_PACKET_LOOP)
	{
		packet->Clear();

		// �ּ��� ��� ũ�⸸ŭ�� �����Ͱ� �Դ��� Ȯ���Ѵ�.
		if (recvCompleteSesion->recvRingBuffer.GetUseSize() < sizeof(Packet::EncodeHeader))
		{
			break;
		}
		
		// ����� �̾ƺ���.
		recvCompleteSesion->recvRingBuffer.Peek((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		if (recvCompleteSesion->recvRingBuffer.GetUseSize() < encodeHeader.packetLen + sizeof(Packet::EncodeHeader))
		{
			break;
		}

		// 1�� ��Ŷ �ڵ��� 52���� �ƴ϶�� ����
		if (encodeHeader.packetCode != 52)
		{
			Disconnect(recvCompleteSesion->sessionId);
			break;
		}

		InterlockedIncrement(&_recvPacketTPS);

		// ������������ �ʹ� ū ��Ŷ�� �ð�� ������ ����
		if (encodeHeader.packetLen > PACKET_BUFFER_DEFAULT_SIZE)
		{
			Disconnect(recvCompleteSesion->sessionId);
			break;
		}

		// ��� ũ�⸸ŭ front�� �����̱�
		recvCompleteSesion->recvRingBuffer.MoveFront(sizeof(Packet::EncodeHeader));
		// ��Ŷ ���̸�ŭ �̾Ƽ� packet�� �ֱ�
		recvCompleteSesion->recvRingBuffer.Dequeue(packet->GetRearBufferPtr(), encodeHeader.packetLen);
		// ��� ����
		packet->SetHeader((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		// ��Ŷ ���� ��ŭ rear �����̱�
		packet->MoveRearPosition(encodeHeader.packetLen);

		// ���ڵ��� �����ϸ� ������ ����
		if (!packet->Decode())
		{
			Disconnect(recvCompleteSesion->sessionId);
			break;
		}	

		// ��Ŷ ó��
		OnRecv(recvCompleteSesion->sessionId, packet);
	}
	
	delete packet;

	RecvPost(recvCompleteSesion);
}

void CoreNetwork::SendComplete(Session* sendCompleteSession)
{
	sendCompleteSession->sendPacket.Clear();	

	// isSend�� 0 ���� �ٲ㼭 WSASend�� �ɼ� �ֵ��� ���ش�.
	InterlockedExchange(&sendCompleteSession->isSend, 0);

	// ���� ������ �ٲٱ� ���� ť�׸� �ϰ� ���� ���
	// ���⼭ ũ�⸦ �˻��ؼ� WSASend�� �ɾ��ش�.
	if (sendCompleteSession->sendQueue.Size() > 0)
	{
		SendPost(sendCompleteSession);
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
	if (findSession->ioBlock->IsRelease == 1)
	{
		return nullptr;
	}	

	// IOCount�� 1 ���������� �� �� ���� 1���� Ȯ���Ѵ�.
	// 1 �̶�� �ش� session�� release ���̰ų� �������� �� �� �ִ�.
	if (InterlockedIncrement64(&findSession->ioBlock->ioCount) == 1)
	{
		// ���⼭ IOCount�� 1 ���ҽ����� �� 0�̶�� release�� �����Ѵ�.
		// �ٸ� ������ IOCount�� �����ϰ�, ���� ReleaseSession�� ȣ���ϱ� ���� �� �����ϱ�.
		if (InterlockedDecrement64(&findSession->ioBlock->ioCount) == 0)
		{
			ReleaseSession(findSession);
		}

		// �ƴ϶�� nullptr�� ��ȯ�� release ������� �˷��ش�.
		return nullptr;
	}

	return findSession;
}

void CoreNetwork::ReturnSession(Session* session)
{
	if (InterlockedDecrement64(&session->ioBlock->ioCount) == 0)
	{
		ReleaseSession(session);
	}
}
