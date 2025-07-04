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

	for (int sessionCount = SERVER_SESSION_MAX - 1; sessionCount >= 0; sessionCount--)
	{
		_sessions[sessionCount] = new Session();
		_sessions[sessionCount]->ioBlock = (IOBlock*)_aligned_malloc(sizeof(IOBlock), 16);
		memset(&_sessions[sessionCount]->sendPacket, 0, sizeof(Packet*) * SESSION_SEND_PACKET_MAX);
		_sessions[sessionCount]->clientSocket = INVALID_SOCKET;
		_sessions[sessionCount]->ioBlock->ioCount = 0;
		_sessions[sessionCount]->ioBlock->IsRelease = 0;

		_sessionIndexs.Push(sessionCount);
	}	
}

CoreNetwork::~CoreNetwork()
{
}

bool CoreNetwork::Start(const WCHAR* openIP, int port)
{
	_ipCountryChecker.Start();

	wcout << L"채팅 서버 시작" << endl;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DWORD error = WSAGetLastError();
		std::cout << "WSAStartup failed : " << error << std::endl;
		return false;
	}

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
	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadProc, this, 0, NULL);	

	// IOCP 워커 스레드 생성
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
	wcout << L"채팅 서버 종료" << endl;	

	// listen 소켓 닫기
	closesocket(_listenSocket);
	
	WSACleanup();
	
	// session 닫기
	for (Session* session : _sessions)
	{
		closesocket(session->clientSocket);
		session->clientSocket = INVALID_SOCKET;
	}	

	// accept 스레드 종료
	CloseHandle(_hAcceptThread);
	_hAcceptThread = NULL;

	for (HANDLE workerThread : _hWorkerThreadHandles)
	{
		CloseHandle(workerThread);
	}

	_hWorkerThreadHandles.clear();

	// IOCP 핸들 닫기
	CloseHandle(_HCP);
	_HCP = NULL;
		
	_acceptTPS = 0;
	_acceptTotal = 0;
	_sessionId = 0;
	_listenSocket = NULL;	
		
	wcout << L"채팅 서버 종료 완료" << endl;
}

void CoreNetwork::SendPacket(__int64 sessionId, Packet* packet)
{	
	Session* sendSession = FindSession(sessionId);
	if (sendSession == nullptr)
	{
		return;
	}

	packet->Encode();
	packet->AddRetCount();

	// 패킷 큐잉
	sendSession->sendQueue.Push(packet);	

	// WSASend 등록
	SendPost(sendSession);

	// FindSession에서 증가시켜준 IOCount를 줄여줌
	ReturnSession(sendSession);
}

void CoreNetwork::Disconnect(__int64 sessionId)
{
	// 연결 종료할 session을 찾음
	Session* disconnectSession = FindSession(sessionId);	
	if (disconnectSession == nullptr)
	{		
		ReturnSession(disconnectSession);
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
	return _sessionCount;
}

unsigned __stdcall CoreNetwork::AcceptThreadProc(void* argument)
{
	CoreNetwork* instance = (CoreNetwork*)argument;

	if (instance != nullptr)
	{
		while(1)
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

			CHAR clientIp[16] = { 0 };
			CHAR clientInfo[50] = { 0 };
			InetNtopA(AF_INET, &clientAddr.sin_addr, clientIp, 16);											

			string nationCode = instance->_ipCountryChecker.IPCheck(clientIp);
			if (nationCode != "LOOPBACK" && nationCode != "KR")
			{
				cout << "Code [" << nationCode << "] close IP: " << clientIp << endl;
				closesocket(clientSock);
				continue;
			}			

			//cout << "NationCode : " << nationCode << " Connect Client IP : " << clientIp << " Port : " << ntohs(clientAddr.sin_port) << endl;

			// 세션 할당
			Session* newSession;
			int newSessionIndex;

			instance->_sessionIndexs.Pop(&newSessionIndex);
			newSession = instance->_sessions[newSessionIndex];
			memset(&newSession->recvOverlapped, 0, sizeof(OVERLAPPED));
			memset(&newSession->sendOverlapped, 0, sizeof(OVERLAPPED));
			newSession->sessionId = MAKE_SESSION_ID(instance->_sessionId, newSessionIndex);
			newSession->clientAddr = clientAddr;
			newSession->clientSocket = clientSock;
			newSession->isSend = SENDING_DO_NOT;

			// IOCP에 등록
			CreateIoCompletionPort((HANDLE)newSession->clientSocket, instance->_HCP, (ULONG_PTR)newSession, 0);

			newSession->ioBlock->ioCount++;
			newSession->ioBlock->IsRelease = 0;

			instance->_sessionId++;

			instance->OnClientJoin(newSession);
			instance->RecvPost(newSession, true);			

			InterlockedIncrement(&instance->_sessionCount);
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
				// myOverlapped가 nullptr이라면 GetQueuedCompletionStatus가 실패한 경우라서 return 처리한다.
				if (myOverlapped == nullptr)
				{
					GQCSError = WSAGetLastError();
					cout << "MyOverlapped NULL " << GQCSError << endl;
					return -1;
				}

				// transferred가 0이라면 fin 패킷을 받은것이므로 종료처리한다.
				if (transferred == 0)
				{
					break;
				}

				// 전달받은 overlapped가 recvOverlapped라면 recv 완료 처리를 진행한다.
				if (myOverlapped == &completeSession->recvOverlapped)
				{
					instance->RecvComplete(completeSession, transferred);
				}
				// 전달받은 overlapped가 sendOverlapped라면 send 완료 처리를 진행한다.
				else if (myOverlapped == &completeSession->sendOverlapped)
				{
					instance->SendComplete(completeSession);
				}
			} while (0);

			// IO 처리가 하나 끝났으므로 IOCount를 감소시킨다.
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
	WSABUF sendBuf[SESSION_SEND_PACKET_MAX];

	do
	{
		// Session의 isSend를 1로 바꾸면서 진입한다.
		// 다른 워커 쓰레드가 WSASend를 하지 않도록 막는다.
		if (InterlockedExchange(&sendSession->isSend, SENDING_DO) == SENDING_DO_NOT)
		{
			sendUseSize = sendSession->sendQueue.Size();

			/*
				보내려고 진입했지만 isSend가 1이라 다른 스레드에서 이미 WSASend 중일 수 있다.
				만약 sendQueue가 비어 있다고 판단하고 isSend를 0으로 바꾸는 순간
				다른 스레드가 패킷을 넣고 SendPost를 호출할 수 있다.
				하지만 isSend가 여전히 1이므로 그 스레드는 WSASend를 시도하지 못하고 빠져나간다.
				결국 현재 스레드도 그냥 빠져나가면 새로 들어온 패킷은 전송되지 않은 채로 남게 된다.
				따라서 isSend를 0으로 바꾼 직후, 큐에 데이터가 들어왔는지를 한번 더 확인하고,
				존재한다면 다시 SendPost를 반복하여 WSASend를 시도한다.

				ex) 일반적인 상황
				A Thread 진입 isSend = 1로 설정하고 진입
				sendQueue.size() == 0 확인 이후 나감				

				A, B Thread 2개
				A Thread 진입 isSend = 1로 설정하고 진입
				sendQueue.size() == 0 확인 isSend = 0 으로 바꾸기 직전
				B Thread가 패킷을 큐잉하고 진입 isSend가 1이라 그냥 나감

				B Thread에서 패킷을 넣었는데 isSend가 1이라 그냥 나가고
				A Thread에서는 sendQueue.size()가 0이 였기 때문에 isSend를 0으로 바꾸고 그냥 나감
				
				결국 큐에 패킷이 남아 있는데 WSASend를 하지 않기 때문에 

				A Thread에서 sendQue를 다시한번 확인해서 SendPost를 다시 진행시킨다.
			*/
			if (sendUseSize == 0)
			{
				InterlockedExchange(&sendSession->isSend, SENDING_DO_NOT);

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

	sendSession->sendPacketCount = sendUseSize;

	// 보내야할 패킷의 개수만큼 sendQueue에서 패킷을 꺼내서 WSABUF에 넣는다.
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

		sendSession->sendPacket[i] = packet;			
	}

	// WSAsend를 걸기 전에 한번 청소해준다.
	memset(&sendSession->sendOverlapped, 0, sizeof(OVERLAPPED));

	// IOCount를 1 증가시켜 해당 session이 release 대상이 되지 않도록 한다.
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

	// release Session에 대해서 더블 CAS를 통해
	// IOCount가 0 인지( 사용중인지 아닌지 확인 )와
	// IsRelease가 true 인지( Release를 한 세션 인지 확인 )를 확인한다.
	if (!InterlockedCompareExchange128((LONG64*)releaseSession->ioBlock, (LONG64)true, (LONG64)0, (LONG64*)&compareBlock))
	{
		return;
	}

	// recvBuf 청소
	releaseSession->recvRingBuffer.ClearBuffer();

	// sendPacket을 호출해서 패킷을 enqueue만 하고 빠질 가능성이 있기 때문에
	// 이 부분에서 sendQueue의 크기를 확인해서 내용이 남아있으면 해제한다.
	while (releaseSession->sendQueue.Size() > 0)
	{
		releaseSession->sendQueue.Clear();		
	}
		
	// SendPost까지 진입해서 Alloc을 받고 session이 보낼 패킷을 저장해둔 SendPacket 배열에 기록은 해뒀으나
	// Release로 진입해서 해제를 못하고 남아있을 경우 여기서 한번 더 확인해서 최종적으로 반납한다.
	for (int i = 0; i < releaseSession->sendPacketCount; i++)
	{
		releaseSession->sendPacket[i]->Free();
		releaseSession->sendPacket[i] = nullptr;
	}

	releaseSession->sendPacketCount = 0;

	closesocket(releaseSession->clientSocket);	
	releaseSession->clientSocket = INVALID_SOCKET;	

	_sessionIndexs.Push(GET_SESSION_INDEX(releaseSession->sessionId));		

	InterlockedDecrement(&_sessionCount);
}

void CoreNetwork::RecvComplete(Session* recvCompleteSesion, const DWORD& transferred)
{
	int loopCount = 0;
	const int MAX_PACKET_LOOP = 64;
	recvCompleteSesion->recvRingBuffer.MoveRear(transferred);

	Packet::EncodeHeader encodeHeader;
	Packet* packet = Packet::Alloc();

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
		if (recvCompleteSesion->recvRingBuffer.GetUseSize() < encodeHeader.packetLen + sizeof(Packet::EncodeHeader))
		{
			break;
		}

		// 1차 패킷 코드인 52값이 아니라면 나감
		if (encodeHeader.packetCode != 52)
		{
			Disconnect(recvCompleteSesion->sessionId);
			break;
		}

		InterlockedIncrement(&_recvPacketTPS);

		// 비정상적으로 너무 큰 패킷이 올경우 연결을 끊음
		if (encodeHeader.packetLen > PACKET_BUFFER_DEFAULT_SIZE)
		{
			Disconnect(recvCompleteSesion->sessionId);
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

		// 디코딩에 실패하면 연결을 끊음
		if (!packet->Decode())
		{
			Disconnect(recvCompleteSesion->sessionId);
			break;
		}	

		// 패킷 처리
		OnRecv(recvCompleteSesion->sessionId, packet);
	}
	
	packet->Free();

	RecvPost(recvCompleteSesion);
}

void CoreNetwork::SendComplete(Session* sendCompleteSession)
{
	for (int i = 0; i < sendCompleteSession->sendPacketCount; i++)
	{
		sendCompleteSession->sendPacket[i]->Free();
	}

	sendCompleteSession->sendPacketCount = 0;	

	// isSend를 0 으로 바꿔서 WSASend를 걸수 있도록 해준다.
	InterlockedExchange(&sendCompleteSession->isSend, SENDING_DO_NOT);

	// 만약 위에서 바꾸기 전에 큐잉만 하고 빠질 경우
	// 여기서 크기를 검사해서 WSASend를 걸어준다.
	if (sendCompleteSession->sendQueue.Size() > 0)
	{
		SendPost(sendCompleteSession);
	}
}

Session* CoreNetwork::FindSession(__int64 sessionId)
{
	int sessionIndex = GET_SESSION_INDEX(sessionId);

	if (_sessions[sessionIndex]->ioBlock->IsRelease == 1)
	{
		return nullptr;
	}		

	// IOCount를 1 증가시켰을 때 그 값이 1인지 확인한다.
	// 1 이라면 해당 session이 release 중이거나 예정임을 알 수 있다.
	if (InterlockedIncrement64(&_sessions[sessionIndex]->ioBlock->ioCount) == 1)
	{
		// 여기서 IOCount를 1 감소시켰을 때 0이라면 release를 진행한다.
		// 다른 곳에서 IOCount가 감소하고, 아직 ReleaseSession을 호출하기 전일 수 있으니까.
		if (InterlockedDecrement64(&_sessions[sessionIndex]->ioBlock->ioCount) == 0)
		{
			ReleaseSession(_sessions[sessionIndex]);
		}

		// 아니라면 nullptr을 반환해 release 대상임을 알려준다.
		return nullptr;
	}

	// sessionId가 다르다면 
	if (sessionId != _sessions[sessionIndex]->sessionId)
	{
		// 더이상 해당 session을 사용하지 않을 것이므로 ioCount를 감소시켜준다.
		if (InterlockedDecrement64(&_sessions[sessionIndex]->ioBlock->ioCount) == 0)
		{
			ReleaseSession(_sessions[sessionIndex]);
		}

		return nullptr;
	}

	return _sessions[sessionIndex];
}

void CoreNetwork::ReturnSession(Session* session)
{
	if (InterlockedDecrement64(&session->ioBlock->ioCount) == 0)
	{
		ReleaseSession(session);
	}
}
