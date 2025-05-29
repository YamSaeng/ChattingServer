#pragma once
#pragma comment(lib,"ws2_32")

#include"Session.h"

// ���� 16��Ʈ�� SessionId�� ����ϰ� ���� 16��Ʈ�� Session�� ��ġ�� �ִ� Index�� ����Ѵ�.
#define MAKE_SESSION_ID(SESSION_ID, INDEX) (((uint32_t)(SESSION_ID) << 16) | ((uint32_t)(INDEX) & 0xFFFF))
// ���� 16��Ʈ���� SessionId�� ������
#define GET_SESSION_ID(SESSION_UID)        ((uint16_t)((SESSION_UID >> 16) & 0xFFFF))
// ���� 16��Ʈ���� SessionIndex�� ������
#define GET_SESSION_INDEX(SESSION_UID)     ((uint16_t)(SESSION_UID & 0xFFFF))

class CoreNetwork
{
public:
	CoreNetwork();
	~CoreNetwork();
private:
	// IOCP �ڵ� 
	HANDLE _HCP;

	// ���� ����
	SOCKET _listenSocket;

	// ������ ������ sessionId;
	INT64 _sessionId;

	// �������� ������ session �迭
	vector<Session*> _sessions;

	// Accept ���� ������ �Լ�
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker ���� ������ �Լ�
	static unsigned __stdcall WorkerThreadProc(void* argument);

	// WSARecv ���
	void RecvPost(Session* recvSession, bool isAcceptRecvPost = false);	

	// IOCount�� 0�� ����� �����Ѵ�.
	void ReleaseSession(Session* releaseSession);
protected:
	// accept ȣ�� �� ������ Ŭ�� ������� ȣ���ϴ� �Լ�
	virtual void OnClientJoin(Session* newSession) = 0;

	// ���� ã��
	Session* FindSession(__int64 sessionId);

public:
	// 1�� ���� ���� ���� ����
	int _acceptTPS;
	// ���� ���� �� ����
	int _acceptTotal;

	// ���� ����
	bool Start(const WCHAR* openIP, int port);
};