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
	// IOCP �ڵ� 
	HANDLE _HCP;

	// ���� ����
	SOCKET _listenSocket;	

	// ������ ������ sessionId;
	INT64 _sessionId;

	// �������� ������ session ����Ʈ
	list<Session*> _sessions;	

	// Accept ���� ������ �Լ�
	static unsigned __stdcall AcceptThreadProc(void* argument);

	// Worker ���� ������ �Լ�
	static unsigned __stdcall WorkerThreadProc(void* argument);	

	// WSARecv ���
	void RecvPost(Session* recvSession);

protected:
	// accept ȣ�� �� ������ Ŭ�� ������� ȣ���ϴ� �Լ�
	virtual void OnClientJoin(Session* newSession) = 0;

public:
	// 1�� ���� ���� ���� ����
	int _acceptTPS;
	// ���� ���� �� ����
	int _acceptTotal;

	// ���� ����
	bool Start(const WCHAR* openIP, int port);
};