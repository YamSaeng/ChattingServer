#pragma once

#include"CoreNetwork.h"

#include"../ObjectPool.h"

#define MAX_PROTOCOL_COUNT 50

struct Job
{	
	__int64 sessionId;
	Packet* packet;
};

class ChattingServer : public CoreNetwork
{
public:	
	ChattingServer();
	~ChattingServer();	
private:
	vector<HANDLE> _hUpdateThreads;
	HANDLE _hUpdateWakeEvent;

	ObjectPool<Job>* _jobPool;

	queue<Job*> _jobQueue;
	CRITICAL_SECTION _jobQueueLock;	

	void (ChattingServer::*packetProc[MAX_PROTOCOL_COUNT])(__int64 sessionId, Packet* packet);
	void PacketProcReqChat(__int64 sessionId, Packet* packet);	
	
	Packet* MakePacketProcResChat(string& chatMessage);

	static unsigned __stdcall UpdateThreadProc(void* argument);	
public:
	volatile __int64 _updateWakeCount; // Update 쓰레드가 깨워진 횟수
	volatile __int64 _updateTPS; // Update 쓰레드가 1초에 작업한 처리량

	void OnClientJoin(Session* newSession) override;
	void OnRecv(__int64 sessionId, Packet* packet) override;
	void OnClientLeave(Session* leaveSession) override;
};