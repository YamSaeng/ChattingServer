#pragma once
#pragma comment(lib,"ws2_32")

#include"Session.h"
#include"../LockfreeStack.h"

#include"IPCountryChecker.h"

// 상위 16비트에 SessionId를 기록하고 하위 16비트에 Session이 위치해 있는 Index를 기록한다.
#define MAKE_SESSION_ID(SESSION_ID, INDEX) (((uint32_t)(SESSION_ID) << 16) | ((uint32_t)(INDEX) & 0xFFFF))
// 상위 16비트에서 SessionId를 가져옴
#define GET_SESSION_ID(SESSION_UID)        ((uint16_t)((SESSION_UID >> 16) & 0xFFFF))
// 하위 16비트에서 SessionIndex를 가져옴
#define GET_SESSION_INDEX(SESSION_UID)     ((uint16_t)(SESSION_UID & 0xFFFF))

#define SENDING_DO_NOT 0
#define SENDING_DO 1

#define SERVER_SESSION_MAX 5000

class CoreNetwork
{
public:
	CoreNetwork();
	virtual ~CoreNetwork();
private:	
	IPCountryChecker _ipCountryChecker;

	// IOCP 핸들 
	HANDLE _HCP;

	// 리슨 소켓
	SOCKET _listenSocket;

	// 서버에 접속한 sessionId;
	INT64 _sessionId;	

	// Accept 스레드 핸들
	HANDLE _hAcceptThread;

	// Worker 스레드 핸들
	vector<HANDLE> _hWorkerThreadHandles;	

	// Accept 전용 스레드 함수
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker 전용 스레드 함수
	static unsigned __stdcall WorkerThreadProc(void* argument);

	// WSARecv 등록
	void RecvPost(Session* recvSession, bool isAcceptRecvPost = false);	

	// WSASend 등록
	void SendPost(Session* sendSession);

	// IOCount가 0인 대상을 해제한다.
	void ReleaseSession(Session* releaseSession);

	// WSARecv 완료시 호출하는 함수
	void RecvComplete(Session* recvCompleteSesion, const DWORD& transferred);

	// WSASend 완료시 호출하는 함수
	void SendComplete(Session* sendCompleteSession);
protected:	
	// 서버에서 관리할 session 배열
	Session* _sessions[SERVER_SESSION_MAX];
	LockfreeStack<int> _sessionIndexs;		

	// accept 호출 후 접속한 클라를 대상으로 호출하는 함수
	virtual void OnClientJoin(Session* newSession) = 0;

	// packet 처리
	virtual void OnRecv(__int64 sessionId, Packet* packet) = 0;

	// 세션 찾기
	Session* FindSession(__int64 sessionId);

	// 증가시킨 IOCount를 1 감소시킨다.
	void ReturnSession(Session* session);

	// ReleaseSession 안에서 반납되는 session을 기준으로 호출
	virtual void OnClientLeave(Session* leaveSession) = 0;
	
public:
	// 1초 동안 연결 수락 개수
	int _acceptTPS;
	// 연결 수락 총 개수
	int _acceptTotal;

	// session 개수
	LONG _sessionCount;

	// recvPacet을 1초 동안 받은 횟수
	LONG _recvPacketTPS;
	// sendPacket을 1초 동안 보낸 횟수
	LONG _sendPacketTPS;

	// 서버 시작
	bool Start(const WCHAR* openIP, int port);

	// 서버 멈춤
	void Stop();

	// 패킷을 전송
	void SendPacket(__int64 sessionId, Packet* packet);

	// 끊기
	void Disconnect(__int64 sessionId);

	// 접속중인 Session 개수
	int SessionCount();
};