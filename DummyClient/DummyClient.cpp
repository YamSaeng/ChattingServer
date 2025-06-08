#include"pch.h"
#include"DummyClient.h"

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

	Packet* packet = new Packet();		
	*packet << make_pair(msg.c_str(), (int)msg.length());

	packet->Encode();	

	sendWsabuf.buf = packet->GetHeaderBufferPtr();
	sendWsabuf.len = packet->GetUseBufferSize();

	_dummyClientSession->sendPacket = packet;	

	memset(&_dummyClientSession->sendOverlapped, 0, sizeof(OVERLAPPED));

	DWORD sent;
	int sendResult = WSASend(_dummyClientSession->clientSocket, &sendWsabuf,1,&sent,0, (LPWSAOVERLAPPED)&_dummyClientSession->sendOverlapped, nullptr);
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

	WSABUF recvWsabuf;
	recvWsabuf.buf = _dummyClientSession->recvBuffer;
	recvWsabuf.len = sizeof(_dummyClientSession->recvBuffer);

	DWORD recvBytes = 0;
	DWORD flags = 0;

	memset(&_dummyClientSession->recvOverlapped, 0, sizeof(OVERLAPPED));

	int recvResult = WSARecv(_dummyClientSession->clientSocket, &recvWsabuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&_dummyClientSession->recvOverlapped, nullptr);
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

	_dummyClientSession->recvBuffer[transferred] = '\0';
	cout << "[Dummy " << _id << "] Received: " << _dummyClientSession->recvBuffer << endl;
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
