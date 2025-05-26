#pragma once
#pragma comment(lib,"ws2_32")

#include"Session.h"

using namespace std;

class CoreNetwork
{
public:
	CoreNetwork();
	~CoreNetwork();
private:
	// IOCP 핸들 
	HANDLE _HCP;

	// 리슨 소켓
	SOCKET _listenSocket;	

	// 서버에 접속한 sessionId;
	INT64 _sessionId;

	// 서버에서 관리할 session 리스트
	list<Session*> _sessions;	

	// Accept 전용 스레드 함수
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker 전용 스레드 함수
	static unsigned __stdcall WorkerThreadProc(void* argument);	

	// WSARecv 등록
	void RecvPost(Session* recvSession);

protected:
	// accept 호출 후 접속한 클라를 대상으로 호출하는 함수
	virtual void OnClientJoin(Session* newSession) = 0;

public:
	// 1초 동안 연결 수락 개수
	int _acceptTPS;
	// 연결 수락 총 개수
	int _acceptTotal;

	// 서버 시작
	bool Start(const WCHAR* openIP, int port);
};