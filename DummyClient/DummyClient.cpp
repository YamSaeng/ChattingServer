#include"pch.h"
#include"DummyClient.h"
#include"../protocol.h"

DummyClient::DummyClient()
{
	_dummyClientSession = nullptr;
	_connected = false;

	_id = -1;
	_clientSocket = INVALID_SOCKET;
	_hIOCP = NULL;
	_waitingReconnect = false;
	_disconnectTime = 0;
	_reconnectInterval = 3000;
	_maxReconnectAttempt = 5;
	_currentReconnectAttempt = 0;
	_isSending = false;

	_serverIp = L"";
	_serverPort = 0;
}

DummyClient::~DummyClient()
{
	if (_dummyClientSession)
	{
		delete _dummyClientSession;
		_dummyClientSession = nullptr;
	}
}

bool DummyClient::Connect(const wchar_t* ip, int port, int id, HANDLE hIOCP)
{
	_id = id;

	_serverIp = ip;
	_serverPort = port;
	_hIOCP = hIOCP;

	_dummyClientSession = new DummyClientSession();
	_dummyClientSession->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_dummyClientSession->clientSocket == INVALID_SOCKET)
	{
		cout << "DummyClient ["<<_id<<"] 소켓 생성 실패: " << WSAGetLastError() << endl;
		return false;
	}

	_dummyClientSession->serverAddr.sin_family = AF_INET;
	_dummyClientSession->serverAddr.sin_port = htons(port);
	InetPton(AF_INET, ip, &_dummyClientSession->serverAddr.sin_addr);

	if(connect(_dummyClientSession->clientSocket, (SOCKADDR*)&_dummyClientSession->serverAddr, sizeof(_dummyClientSession->serverAddr)) == SOCKET_ERROR)
	{
		cout << "DummyClient ["<<_id<<"] connect 실패: " << WSAGetLastError() << endl;

		closesocket(_dummyClientSession->clientSocket);
		_dummyClientSession->clientSocket = INVALID_SOCKET;
		return false;
	}

	_dummyClientSession->ioBlock = new IOBlock();
	_connected = true;
	_waitingReconnect = false;
	_currentReconnectAttempt = 0;

	CreateIoCompletionPort((HANDLE)_dummyClientSession->clientSocket, hIOCP, (ULONG_PTR)this, 0);

	_dummyClientSession->ioBlock->ioCount++;
	_dummyClientSession->ioBlock->isRelease = 0;

	RecvPost(true);

	return true;
}

void DummyClient::Disconnect(void)
{
	if (_connected == true)
	{	
		CancelIoEx((HANDLE)_dummyClientSession->clientSocket, NULL);
	}		
}

bool DummyClient::ReconnectTimeCheck(void)
{
	if (_waitingReconnect == false)
	{
		return false;
	}

	if (_currentReconnectAttempt >= _maxReconnectAttempt)
	{
		cout << "DummyClient [" << _id << "] 최대 재접속 시도 횟수 초과" << endl;
		return false;
	}

	DWORD currentTime = GetTickCount64();
	if (currentTime - _disconnectTime >= _reconnectInterval)
	{
		return true;
	}

	return false;
}

void DummyClient::ReconnectTry(void)
{
	if (_waitingReconnect == false)
	{
		return;
	}

	if (_currentReconnectAttempt >= _maxReconnectAttempt)
	{
		cout << "DummyClient [" << _id << "] 최대 재접속 시도 횟수 초과" << endl;
		_waitingReconnect = false;
		return;
	}

	_currentReconnectAttempt++;
	cout << "DummyClient [" << _id << "] 재접속 시도 중... (" << _currentReconnectAttempt << "/" << _maxReconnectAttempt << ")" << endl;

	bool reconnectResult = Connect(_serverIp.c_str(), _serverPort, _id, _hIOCP);
	if (reconnectResult)
	{
		cout << "DummyClient [" << _id << "] 재접속 성공!" << endl;
		_waitingReconnect = false;
		_currentReconnectAttempt = 0;
	}
	else
	{
		cout << "DummyClient [" << _id << "] 재접속 실패. 다시 시도 예정..." << endl;
		_disconnectTime = GetTickCount64(); // 실패한 시간으로 갱신하여 다음 재접속 시도 시간 조정

		// 재접속 실패시 간격을 점진적으로 늘릴 수도 있음 (지수 백오프)
		_reconnectInterval = min(_reconnectInterval * 2, 30000); // 최대 30초
	}
}

void DummyClient::ReleaseDummyClient()
{
	IOBlock compareIOBlock;
	compareIOBlock.ioCount = 0;

	if (!InterlockedCompareExchange128((LONG64*)_dummyClientSession->ioBlock,
		(LONG64)true, (LONG64)0,
		(LONG64*)&compareIOBlock))
	{
		return;
	}

	cout << "DummyClient [" << _id << "] 연결 끊기 " << endl;

	_connected = false;
	_waitingReconnect = true;
	_disconnectTime = GetTickCount64();

	closesocket(_dummyClientSession->clientSocket);
	_dummyClientSession->clientSocket = INVALID_SOCKET;

	if (_dummyClientSession && _dummyClientSession->sendPacket != nullptr)
	{
		_dummyClientSession->sendPacket->Free();
		_dummyClientSession->sendPacket = nullptr;
	}

	delete _dummyClientSession->ioBlock;
	_dummyClientSession->ioBlock = nullptr;
	delete _dummyClientSession;	
	_dummyClientSession = nullptr;	
}

void DummyClient::SendRandomMessage(void)
{
	if (_connected == false)
	{
		return;
	}

	if (!InterlockedExchange(&_isSending, 1) == 0)
	{
		return;
	}	

	WSABUF sendWsabuf;

	const char* samples[] = {
	"Hello", "Stress Test","Test Send", "Jung", "Bye"};
	string msg = samples[rand() % (sizeof(samples) / sizeof(char*))];	
		
	Packet* c2sChatPacket = Packet::Alloc();
	c2sChatPacket->Clear();

	short packetType = (short)en_CHATTING_SERVER_PACKET_C2S_CHAT;	
	*c2sChatPacket << packetType;
	*c2sChatPacket << make_pair(msg.c_str(), (int)msg.length());

	c2sChatPacket->Encode();	
	c2sChatPacket->AddRetCount();

	sendWsabuf.buf = c2sChatPacket->GetHeaderBufferPtr();
	sendWsabuf.len = c2sChatPacket->GetUseBufferSize();	

	_dummyClientSession->sendPacket = c2sChatPacket;	

	memset(&_dummyClientSession->sendOverlapped, 0, sizeof(OVERLAPPED));

	InterlockedIncrement64(&_dummyClientSession->ioBlock->ioCount);

	int sendResult = WSASend(_dummyClientSession->clientSocket, &sendWsabuf, 1, NULL, 0,
		(LPWSAOVERLAPPED)&_dummyClientSession->sendOverlapped, nullptr);
	if (sendResult == SOCKET_ERROR)
	{
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			cout << "DummyClient [" << _id << "] WSASend fail" << error << endl;
			Disconnect();
		}
	}	

	c2sChatPacket->Free();
}

void DummyClient::RecvPost(bool isConnectRecvPost)
{
	if (_connected == false)
	{
		return;
	}

	int recvBuffCount = 0;
	WSABUF recvWsabuf[2];

	int directEnqueueSize = _dummyClientSession->recvRingBuffer.GetDirectEnqueueSize();
	int recvRingBuffFreeSize = _dummyClientSession->recvRingBuffer.GetFreeSize();

	if (recvRingBuffFreeSize > directEnqueueSize)
	{
		recvBuffCount = 2;
		recvWsabuf[0].buf = _dummyClientSession->recvRingBuffer.GetRearBufferPtr();
		recvWsabuf[0].len = directEnqueueSize;

		recvWsabuf[1].buf = _dummyClientSession->recvRingBuffer.GetBufferPtr();
		recvWsabuf[1].len = recvRingBuffFreeSize - directEnqueueSize;
	}
	else
	{
		recvBuffCount = 1;
		recvWsabuf[0].buf = _dummyClientSession->recvRingBuffer.GetRearBufferPtr();
		recvWsabuf[0].len = directEnqueueSize;
	}	

	memset(&_dummyClientSession->recvOverlapped, 0, sizeof(OVERLAPPED));

	if (isConnectRecvPost == false)
	{
		InterlockedIncrement64(&_dummyClientSession->ioBlock->ioCount);
	}

	DWORD flags = 0;
	int recvResult = WSARecv(_dummyClientSession->clientSocket, recvWsabuf, recvBuffCount, NULL, &flags, (LPWSAOVERLAPPED)&_dummyClientSession->recvOverlapped, NULL);
	if (recvResult == SOCKET_ERROR)
	{
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			if (error != WSA_IO_PENDING)
			{
				cout << "DummyClient [" << _id << "] WSARecv fail" << error << endl;
				Disconnect();
			}
		}
	}
}

void DummyClient::RecvComplete(DWORD transferred)
{
	if (transferred == 0)
	{
		Disconnect();
		return;
	}	

	_dummyClientSession->recvRingBuffer.MoveRear(transferred);

	int loopCount = 0;
	const int MAX_PACKET_LOOP = 64;
	
	Packet::EncodeHeader encodeHeader;
	Packet* packet = Packet::Alloc();

	while (loopCount++ < MAX_PACKET_LOOP)
	{
		packet->Clear();

		if (_dummyClientSession->recvRingBuffer.GetUseSize() < sizeof(Packet::EncodeHeader))
		{
			break;
		}			

		_dummyClientSession->recvRingBuffer.Peek((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		if (_dummyClientSession->recvRingBuffer.GetUseSize() < encodeHeader.packetLen + sizeof(Packet::EncodeHeader))
		{
			break;
		}

		if (encodeHeader.packetCode != 52)
		{
			Disconnect();
			break;
		}

		_dummyClientSession->recvRingBuffer.MoveFront(sizeof(Packet::EncodeHeader));
		_dummyClientSession->recvRingBuffer.Dequeue(packet->GetRearBufferPtr(), encodeHeader.packetLen);
		packet->SetHeader((char*)&encodeHeader, sizeof(Packet::EncodeHeader));
		packet->MoveRearPosition(encodeHeader.packetLen);

		if (!packet->Decode())
		{
			Disconnect();
			break;
		}		

		int chatLen;
		*packet >> chatLen;

		string chatMessage;
		chatMessage.resize(chatLen);
		*packet >> make_pair(chatMessage.data(), chatLen);

		//cout << "[Dummy " << _id << "] Received: " << chatMessage.c_str() << endl;
	}		
	
	packet->Free();

	RecvPost(false);
}

void DummyClient::SendComplete(void)
{
	if (_dummyClientSession->sendPacket != nullptr)
	{
		_dummyClientSession->sendPacket->Free();	
		_dummyClientSession->sendPacket = nullptr;
	}
	
	InterlockedExchange(&_isSending, 0);
}
