#pragma once
#pragma comment(lib,"ws2_32")

#include"DummyClientSession.h"

class DummyClient
{
public:
	DummyClient();
	~DummyClient();

	DummyClientSession* _dummyClientSession;
	bool _connected;
private:
	int _id;
	SOCKET _clientSocket;
	HANDLE _hIOCP;

	bool _waitingReconnect; // ������ ��� ����
	DWORD _disconnectTime; // ���� ���� �ð�
	DWORD _reconnectInterval; // ������ �õ� ����
	int _maxReconnectAttempt; // �ִ� ������ �õ� Ƚ��
	int _currentReconnectAttempt; // ���� ������ �õ� Ƚ��

	LONG _isSending; // WSASend�ϰ� �ִ��� ����		

	// �������Ҷ� ���� ip, port ����
	wstring _serverIp;
	int _serverPort;	

public:
	bool Connect(const wchar_t* ip, int port, int id, HANDLE hIOCP);
	void Disconnect(void);
	bool ReconnectTimeCheck(void);
	void ReconnectTry(void);
	void ReleaseDummyClient();

	void SendRandomMessage(void);
	void RecvPost(bool isConnectRecvPost);
	void RecvComplete(DWORD transferred);
	void SendComplete(void);
};