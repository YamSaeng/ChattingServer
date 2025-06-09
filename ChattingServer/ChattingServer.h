#pragma once

#include"CoreNetwork.h"

#define PACKET_PROC_COUNT 5

class ChattingServer : public CoreNetwork
{
public:	
	ChattingServer();
	~ChattingServer();	
private:
	void (ChattingServer::*packetProc[PACKET_PROC_COUNT])(__int64 sessionId, Packet* packet);
	void PacketProcReqChat(__int64 sessionId, Packet* packet);
public:
	void OnClientJoin(Session* newSession) override;
	void OnRecv(__int64 sessionId, Packet* packet) override;
	void OnClientLeave(Session* leaveSession) override;
};