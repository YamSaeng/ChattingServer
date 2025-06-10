#pragma once

#include"../Packet.h"
#include"../RingBuffer.h"
#include"../SendQueue.h"

#pragma comment(lib, "..\\x64\\Debug\\NetworkLib.lib")

struct IOBlock
{
	// IO �۾� Ƚ���� ����ص� ���� 
	// Session�� ����ϰ� �ִ����� ���� ����
	LONG64 ioCount = 0;
	
	// Release�� �ߴ��� ���ߴ����� ���� ����
	LONG64 IsRelease = false;
};

struct Session
{
	LONG sessionId = 0;
	SOCKET clientSocket; // Ŭ���̾�Ʈ ����
	
	SOCKADDR_IN clientAddr; // ������ ������ Ŭ�� �ּ�

	RingBuffer recvRingBuffer; // recvBuf
	SendQueue sendQueue; // sendQueue	

	OVERLAPPED recvOverlapped = {}; // WSARecv ����
	OVERLAPPED sendOverlapped = {};	// WSASend ����

	IOBlock* ioBlock = nullptr; // Session�� ���������, �����Ǿ����� Ȯ��
	
	LONG isSend; // ���ǿ� ���� WSASend �۾��� �ϰ� �ִ��� ���ϰ� �ִ��� ����

	SendQueue sendPacket; // ������ ��Ŷ���� ��Ƶ�
	LONG sendPacketCount; // ������ ��Ŷ�� ������ ���
};
