#include "pch.h"
#include "ChattingServer.h"
#include "../protocol.h"

ChattingServer::ChattingServer()
{
	packetProc[en_CHATTING_SERVER_PACKET_C2S_CHAT] = &ChattingServer::PacketProcReqChat;
}

ChattingServer::~ChattingServer()
{
}

void ChattingServer::PacketProcReqChat(__int64 sessionId, Packet* packet)
{

}

void ChattingServer::OnClientJoin(Session* newSession)
{
}

void ChattingServer::OnRecv(__int64 sessionId, Packet* packet)
{
	short protocol;
	*packet >> protocol;

	(this->*packetProc[protocol])(sessionId, packet);
}

void ChattingServer::OnClientLeave(Session* leaveSession)
{
}

