#pragma once

enum en_CHATTING_SERVER_PACKET_TYPE
{
	en_CHATTING_SERVER_PACKET = 0,

	//=======================================
	// ��Ʈ��Ʈ ��û packet
	// short Type
	//=======================================
	en_CHATTING_SERVER_PACKET_C2S_HEARTBEAT = 1,

	//=======================================
	// ��Ʈ��Ʈ ���� packet
	// short Type
	//=======================================
	en_CHATTING_SERVER_PACKET_S2C_HEARTBEAT = 2,

	//=======================================
	// ä�� ��û packet
	// short Type
	// string Message
	//=======================================
	en_CHATTING_SERVER_PACKET_C2S_CHAT = 3,

	//=======================================
	// ä�� ���� packet
	// short Type
	// string Message
	//=======================================
	en_CHATTING_SERVER_PACKET_S2C_CHAT = 4,

	//========================================================
	// ä��ä�� ���� ��û packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_C2S_CREATE_CHATTING_CHANNEL = 5,

	//========================================================
	// ä��ä�� ���� ���� packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_S2C_CREATE_CHATTING_CHANNEL = 6,

	//======================================================
	// ä��ä�� ���� ��û packet
	// short Type
	// string ChannelName
	//======================================================
	en_CHATTING_SERVER_PACKET_C2S_JOIN_CHATTING_CHANNEL = 7,

	//======================================================
	// ä��ä�� ���� ���� packet
	// short Type
	// string ChannelName
	//======================================================
	en_CHATTING_SERVER_PACKET_S2C_JOIN_CHATTING_CHANNEL = 8,

	//========================================================
	// ä��ä�� ������ ��û packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_C2S_LEAVE_CHATTING_CHANNEL = 9,

	//========================================================
	// ä��ä�� ������ ���� packet
	// short Type	
	//========================================================
	en_CHATTING_SERVER_PACKET_S2C_LEAVE_CHATTING_CHANNEL = 10,

	//================================================
	// ä��ä�� ���� ��û packet
	// short Type
	// string ChannelName
	//================================================
	en_CHATTING_SERVER_PACKET_C2S_DELETE_CHANNEL = 11,

	//================================================
	// ä��ä�� ���� ���� packet
	// short Type
	// string ChannelName
	//================================================
	en_CHATTING_SERVER_PACKET_S2C_DELETE_CHANNEL = 12,

	//=======================================================
	// ä��ä�� ��� ��û packet
	// short Type	
	//=======================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_LIST = 13,

	//=======================================================
	// ä��ä�� ��� ���� packet
	// short Type
	// string[] ChannelList
	//=======================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_LIST = 14,

	//==============================================================
	// ä��ä�� ������ ��� ��û packet
	// short Type	
	//==============================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_MEMBER_LIST = 15,

	//==============================================================
	// ä��ä�� ������ ��� ���� packet
	// short Type	
	// string[] MemberList
	//==============================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_MEMBER_LIST = 16,

	//=============================================================
	// ä��ä�� ������ �߰� ��û packet
	// short Type
	// string ChannelName
	// string MemberName
	//=============================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_ADD_MEMBER = 17,

	//=============================================================
	// ä��ä�� ������ �߰� ���� packet
	// short Type
	// string ChannelName
	// string MemberName
	//=============================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_ADD_MEMBER = 18,

	//================================================================
	// ä��ä�� ������ ���� ��û packet
	// short Type
	// string ChannelName
	// string MemberName
	//================================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_REMOVE_MEMBER = 19,

	//================================================================
	// ä��ä�� ������ ���� ���� packet
	// short Type
	// string ChannelName
	// string MemberName
	//================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_REMOVE_MEMBER = 20,

	//=======================================================================
	// ä��ä�� ������ ���� ���� ��û packet
	// short Type
	// string ChannelName
	// string MemberName
	// int Status (0: offline, 1: online)
	//=======================================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_MEMBER_STATUS_CHANGE = 21,

	//=======================================================================
	// ä��ä�� ������ ���� ���� ���� packet
	// short Type
	// string ChannelName
	// string MemberName
	// int Status (0: offline, 1: online)
	//=======================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_MEMBER_STATUS_CHANGE = 22,

	//=================================================================
	// ä��ä�� �޽��� �˸� ���� packet
	// short Type
	// string ChannelName	
	// string Message
	//=================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_GLOBAL_MESSAGE = 23,
};