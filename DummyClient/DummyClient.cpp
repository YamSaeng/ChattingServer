#include"pch.h"
#include"DummyClient.h"
#include"../protocol.h"

DummyClient::DummyClient()
{
	_dummyClientSession = nullptr;
	_isSending = false;
	_connected = false;
	_id = -1;
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

	_dummyClientSession = new DummyClientSession();
	_dummyClientSession->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_dummyClientSession->clientSocket == INVALID_SOCKET)
	{
		cout << "DummyClient socket create fail" << WSAGetLastError() << endl;
		return false;
	}

	_dummyClientSession->serverAddr.sin_family = AF_INET;
	_dummyClientSession->serverAddr.sin_port = htons(port);
	InetPton(AF_INET, ip, &_dummyClientSession->serverAddr.sin_addr);

	if(connect(_dummyClientSession->clientSocket, (SOCKADDR*)&_dummyClientSession->serverAddr, sizeof(_dummyClientSession->serverAddr)) == SOCKET_ERROR)
	{
		cout << "connect fail " << WSAGetLastError() << endl;

		closesocket(_dummyClientSession->clientSocket);
		_dummyClientSession->clientSocket = INVALID_SOCKET;
		return false;
	}

	_connected = true;

	CreateIoCompletionPort((HANDLE)_dummyClientSession->clientSocket, hIOCP, (ULONG_PTR)this, 0);

	RecvPost();

	return true;
}

void DummyClient::Disconnect(void)
{
	if (_dummyClientSession->clientSocket != INVALID_SOCKET)
	{
		closesocket(_dummyClientSession->clientSocket);
		_dummyClientSession->clientSocket = INVALID_SOCKET;
	}

	_connected = false;
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
		
	Packet* c2sChatPacket = new Packet();	
	short packetType = (short)en_CHATTING_SERVER_PACKET_C2S_CHAT;	
	*c2sChatPacket << packetType;
	*c2sChatPacket << make_pair(msg.c_str(), (int)msg.length());

	c2sChatPacket->Encode();

	sendWsabuf.buf = c2sChatPacket->GetHeaderBufferPtr();
	sendWsabuf.len = c2sChatPacket->GetUseBufferSize();

	_dummyClientSession->sendPacket = c2sChatPacket;

	memset(&_dummyClientSession->sendOverlapped, 0, sizeof(OVERLAPPED));

	DWORD sent;
	int sendResult = WSASend(_dummyClientSession->clientSocket, &sendWsabuf,1,&sent,0,
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
}

void DummyClient::RecvPost(void)
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
	Packet* packet = new Packet();

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

		cout << "[Dummy " << _id << "] Received: " << chatMessage.c_str() << endl;
	}		
	
	delete packet;	
	RecvPost();
}

void DummyClient::SendComplete(void)
{
	if (_dummyClientSession->sendPacket != nullptr)
	{
		delete _dummyClientSession->sendPacket;
		_dummyClientSession->sendPacket = nullptr;
	}
	
	InterlockedExchange(&_isSending, 0);
}
