#pragma once
#include<WinSock2.h>

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // Ŭ���̾�Ʈ ����
	
	SOCKADDR_IN clientAddr; // ������ ������ Ŭ�� �ּ�

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};
