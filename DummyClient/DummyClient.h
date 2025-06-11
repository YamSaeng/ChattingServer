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

	bool _waitingReconnect; // 재접속 대기 여부
	DWORD _disconnectTime; // 연결 끊긴 시간
	DWORD _reconnectInterval; // 재접속 시도 간격
	int _maxReconnectAttempt; // 최대 재접속 시도 횟수
	int _currentReconnectAttempt; // 현재 재접속 시도 횟수

	LONG _isSending; // WSASend하고 있는지 여부		

	// 재접속할때 서버 ip, port 정보
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