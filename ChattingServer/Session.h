#pragma once
#include<WinSock2.h>

struct st_Session
{
	LONG sessionId;
	SOCKET clientSocket; // Ŭ���̾�Ʈ ����
	
	SOCKADDR_IN clientAddr; // ������ ������ Ŭ�� �ּ�

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};
