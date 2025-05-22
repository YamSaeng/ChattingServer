#pragma once
#include<WinSock2.h>

struct st_Session
{
	LONG sessionId;
	SOCKET clientSocket; // 클라이언트 소켓
	
	SOCKADDR_IN clientAddr; // 서버에 접속한 클라 주소

	OVERLAPPED recvOverlapped = {};
	OVERLAPPED sendOverlapped = {};	
};
