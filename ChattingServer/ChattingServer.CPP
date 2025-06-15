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
	int chatLen;
	*packet >> chatLen;

	string chatMessage;	
	chatMessage.resize(chatLen);
	*packet >> make_pair(chatMessage.data(), chatLen);

	short packetType = (short)en_CHATTING_SERVER_PACKET_S2C_CHAT;
		
	for (auto session : _sessions)
	{
		Packet* s2cChatPacket = MakePacketProcResChat(chatMessage);
		if (s2cChatPacket == nullptr)
		{
			continue;
		}

		SendPacket(session->sessionId, s2cChatPacket);
		s2cChatPacket->Free();
	}
}

Packet* ChattingServer::MakePacketProcResChat(string& chatMessage)
{
	Packet* s2cChatPacket = Packet::Alloc();
	if (s2cChatPacket == nullptr)
	{
		return nullptr;
	}

	s2cChatPacket->Clear();
	
	short packetType = (short)en_CHATTING_SERVER_PACKET_S2C_CHAT;
	int chatLen = (int)chatMessage.length();
	*s2cChatPacket << make_pair(chatMessage.c_str(), chatLen);

	return s2cChatPacket;
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

