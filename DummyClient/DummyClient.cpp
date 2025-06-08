#include"pch.h"
#include"DummyClient.h"

DummyClient::DummyClient()
{
	memset(&_dummyClientSession, 0, sizeof(DummyClientSession));	
}

DummyClient::~DummyClient()
{
	
}

bool DummyClient::Connect(const wchar_t* ip, int port, int id, HANDLE hIOCP)
{
	_dummyClientSession.clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	_dummyClientSession.serverAddr.sin_family = AF_INET;
	_dummyClientSession.serverAddr.sin_port = htons(port);
	InetPton(AF_INET, ip, &_dummyClientSession.serverAddr.sin_addr);

	if(connect(_dummyClientSession.clientSocket, (SOCKADDR*)&_dummyClientSession.serverAddr, sizeof(_dummyClientSession.serverAddr)) == SOCKET_ERROR) 
	{
		DWORD error = WSAGetLastError();

		cout << "connect 에러 " << error << endl;

		closesocket(_dummyClientSession.clientSocket);
		_dummyClientSession.clientSocket = INVALID_SOCKET;
		return false;
	}

	_connected = true;

	CreateIoCompletionPort((HANDLE)_dummyClientSession.clientSocket, hIOCP, (ULONG_PTR)this, 0);	

	RecvPost();

	return true;
}

void DummyClient::Disconnect(void)
{
	if (_dummyClientSession.clientSocket != INVALID_SOCKET)
	{
		closesocket(_dummyClientSession.clientSocket);
		_dummyClientSession.clientSocket = INVALID_SOCKET;
	}

	_connected = false;
}

void DummyClient::SendRandomMessage(void)
{
	if (_connected == false)
	{
		return;
	}

	string msg = "더미 메세지";
	memcpy(_dummyClientSession.sendBuffer, msg.c_str(), msg.length());

	WSABUF sendWsabuf;
	sendWsabuf.buf = _dummyClientSession.sendBuffer;
	sendWsabuf.len = (ULONG)msg.length();

	memset(&_dummyClientSession.sendOverlapped, 0, sizeof(OVERLAPPED));

	DWORD sent;
	WSASend(_dummyClientSession.clientSocket, &sendWsabuf,1,&sent,0, (LPWSAOVERLAPPED)&_dummyClientSession.sendOverlapped, nullptr);
}

void DummyClient::RecvPost(void)
{
	if (_connected == false)
	{
		return;
	}

	WSABUF recvWsabuf;
	recvWsabuf.buf = _dummyClientSession.recvBuffer;
	recvWsabuf.len = sizeof(_dummyClientSession.recvBuffer);

	DWORD recvBytes = 0;
	DWORD flags = 0;

	memset(&_dummyClientSession.recvOverlapped, 0, sizeof(OVERLAPPED));

	WSARecv(_dummyClientSession.clientSocket, &recvWsabuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&_dummyClientSession.recvOverlapped, nullptr);
}

void DummyClient::RecvComplete(DWORD transferred)
{
	if (transferred == 0)
	{
		Disconnect();
		return;
	}

	_dummyClientSession.recvBuffer[transferred] = '\0';
	cout << "[Dummy " << _id << "] Received: " << _dummyClientSession.recvBuffer << endl;
	RecvPost();
}