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
	// 리슨 소켓 생성
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed" << std::endl;
		return false;
	}

	// 바인드
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

	// 리슨
	int listenResult = listen(_listenSocket, SOMAXCONN);
	if (listenResult == SOCKET_ERROR)
	{
		std::cout << "listen failed" << std::endl;
		return false;
	}

	// IOCP Port를 생성한다.
	_HCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_HCP == NULL)
	{
		std::cout << "CreateIoCompletionPort failed" << std::endl;
		return false;
	}

	// accept 스레드 생성
	HANDLE hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadProc, this, 0, NULL);
	CloseHandle(hAcceptThread);

	// IOCP 워커 스레드 생성
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);

	for (int i = 0; i < (int)SI.dwNumberOfProcessors; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, this, 0, NULL);
		CloseHandle(hWorkerThread);
	}
}

void CoreNetwork::SendPacket(__int64 sessionId, Packet* packet)
{	
	Session* sendSession = FindSession(sessionId);
	if (sendSession == nullptr)
	{
		return;
	}

	packet->Encode();

	// 패킷 큐잉
	sendSession->sendQueue.push(packet);

	// WSASend 등록
	SendPost(sendSession);

	// FindSession에서 증가시켜준 IOCount를 줄여줌
	ReturnSession(sendSession);
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

			// 클라 연결 대기 
			SOCKET clientSock = accept(instance->_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSock == INVALID_SOCKET)
			{
				DWORD error = WSAGetLastError();
				std::cout << "accept failed : " << error << std::endl;
				break;
			}

			// 연결 수락 총 개수 증가
			instance->_acceptTotal++;
			instance->_acceptTPS++;

			// 세션 할당
			Session* newSession = new Session();			

			newSession->sessionId = ++instance->_sessionId;
			newSession->clientAddr = clientAddr;
			newSession->clientSocket = clientSock;

			newSession->IOBlock = new IOBlock();			

			// IOCP에 등록
			CreateIoCompletionPort((HANDLE)newSession->clientSocket, instance->_HCP, (ULONG_PTR)newSession, 0);

			newSession->IOBlock->IOCount++;
			newSession->IOBlock->IsRelease = 0;

			instance->OnClientJoin(newSession);
			instance->RecvPost(newSession);

			// sessions에 저장
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
				if (myOverlapped == nullptr)
				{
					GQCSError = WSAGetLastError();
					wcout << L"MyOverlapped NULL " << GQCSError << endl;
					return -1;
				}

				if (transferred == 0)
				{
					break;
				}

				if (myOverlapped == &completeSession->recvOverlapped)
				{
					instance->RecvComplete(completeSession, transferred);
				}
				else if (myOverlapped == &completeSession->sendOverlapped)
				{
					instance->SendComplete(completeSession);
				}
			} while (0);

			if (InterlockedDecrement64(&completeSession->IOBlock->IOCount) == 0)
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

	// recvRingBuffer에 한번에 넣을 수 있는 크기를 읽는다.
	int directEnqueueSize = recvSession->recvRingBuffer.GetDirectEnqueueSize();
	// 남아 있는 recvRingBuffer의 크기를 읽는다.
	int recvRingBuffFreeSize = recvSession->recvRingBuffer.GetFreeSize();

	// 만약 남아 있는 recvRingBuffer의 크기가 한번에 넣을 수 있는 크기보다 크다면
	// recvRingBuffer가 2개의 공간으로 나뉘어 있는 것을 확인할 수 있다.
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

	// WSARecv를 걸기 전에 한번 청소해준다.
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

void CoreNetwork::SendPost(Session* sendSession)
{

}

void CoreNetwork::ReleaseSession(Session* releaseSession)
{
	IOBlock compareBlock;
	compareBlock.IOCount = 0;
	compareBlock.IsRelease = 0;

	// release Session에 대해서 더블 CAS를 통해
	// IOCount가 0 인지( 사용중인지 아닌지 확인 )와
	// IsRelease가 true 인지( Release를 한 세션 인지 확인 )를 확인한다.
	if (!InterlockedCompareExchange128((LONG64*)releaseSession->IOBlock, (LONG64)true, (LONG64)0, (LONG64*)&compareBlock))
	{
		return;
	}

	// recvBuf 청소
	releaseSession->recvRingBuffer.ClearBuffer();

	// sendPacket을 호출해서 패킷을 enqueue만 하고 빠질 가능성이 있기 때문에
	// 이 부분에서 sendQueue의 크기를 확인해서 내용이 남아있으면 해제한다.
	while (releaseSession->sendQueue.size() > 0)
	{
		Packet* deletePacket = releaseSession->sendQueue.front();
		releaseSession->sendQueue.pop();
		if (deletePacket == nullptr)
		{
			continue;
		}

		delete deletePacket;
	}
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

		// 최소한 헤더 크기만큼은 데이터가 왔는지 확인한다.
		if (recvCompleteSesion->recvRingBuffer.GetUseSize() < sizeof(Packet::EncodeHeader))
		{
			break;
		}

		// 헤더를 뽑아본다.
		recvCompleteSesion->recvRingBuffer.Peek((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		if (encodeHeader.packetLen + sizeof(Packet::EncodeHeader) > recvCompleteSesion->recvRingBuffer.GetUseSize())
		{
			// 1차 패킷 코드인 52값이 아니라면 나감
			if (encodeHeader.packetCode != 52)
			{
				break;
			}			
		}

		// 비정상적으로 너무 큰 패킷이 올경우
		if (encodeHeader.packetLen > PACKET_BUFFER_DEFAULT_SIZE)
		{
			break;
		}

		// 헤더 크기만큼 front를 움직이기
		recvCompleteSesion->recvRingBuffer.MoveFront(sizeof(Packet::EncodeHeader));
		// 패킷 길이만큼 뽑아서 packet에 넣기
		recvCompleteSesion->recvRingBuffer.Dequeue(packet->GetRearBufferPtr(), encodeHeader.packetLen);
		// 헤더 설정
		packet->SetHeader((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		// 패킷 길이 만큼 rear 움직이기
		packet->MoveRearPosition(encodeHeader.packetLen);

		if (packet->Decode() == false)
		{
			break;
		}	

		// 패킷 처리
		OnRecv(recvCompleteSesion->sessionId, packet);
	}
	
	delete packet;
}

void CoreNetwork::SendComplete(Session* sendCompleteSession)
{
	// 전송 완료된 패킷을 정리한다.
	for (int i = 0; i < sendCompleteSession->sendPacketCount; i++)
	{
		delete sendCompleteSession->sendPacket[i];
	}

	sendCompleteSession->sendPacketCount = 0;

	// isSend를 0 으로 바꿔서 WSASend를 걸수 있도록 해준다.
	InterlockedExchange(&sendCompleteSession->isSend, 0);

	// 만약 위에서 바꾸기 전에 큐잉만 하고 빠질 경우
	// 여기서 크기를 검사해서 WSASend를 걸어준다.
	if (sendCompleteSession->sendQueue.size() > 0)
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

	// 만약 session이 release 작업 예정이라면 나간다.
	if (findSession->IOBlock->IsRelease == 1)
	{
		return nullptr;
	}	

	// IOCount를 1 증가시켰을 때 그 값이 1인지 확인한다.
	// 1 이라면 해당 session이 release 중이거나 예정임을 알 수 있다.
	if (InterlockedIncrement64(&findSession->IOBlock->IOCount) == 1)
	{
		// 여기서 IOCount를 1 감소시켰을 때 0이라면 release를 진행한다.
		// 다른 곳에서 IOCount가 감소하고, 아직 ReleaseSession을 호출하기 전일 수 있으니까.
		if (InterlockedDecrement64(&findSession->IOBlock->IOCount) == 0)
		{
			ReleaseSession(findSession);
		}

		// 아니라면 nullptr을 반환해 release 대상임을 알려준다.
		return nullptr;
	}

	return findSession;
}

void CoreNetwork::ReturnSession(Session* session)
{
	if (InterlockedDecrement64(&session->IOBlock->IOCount) == 0)
	{
		ReleaseSession(session);
	}
}
