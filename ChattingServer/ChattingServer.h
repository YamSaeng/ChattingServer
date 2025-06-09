#pragma once

#include"CoreNetwork.h"

#define MAX_PROTOCOL_COUNT 50

class ChattingServer : public CoreNetwork
{
public:	
	ChattingServer();
	~ChattingServer();	
private:
	void (ChattingServer::*packetProc[MAX_PROTOCOL_COUNT])(__int64 sessionId, Packet* packet);
	void PacketProcReqChat(__int64 sessionId, Packet* packet);	
	
	Packet* MakePacketProcResChat(string& chatMessage);
public:
	void OnClientJoin(Session* newSession) override;
	void OnRecv(__int64 sessionId, Packet* packet) override;
	void OnClientLeave(Session* leaveSession) override;
};