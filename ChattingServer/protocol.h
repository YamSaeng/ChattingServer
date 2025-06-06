#pragma once

enum en_CHATTING_SERVER_PACKET_TYPE
{
	en_CHATTING_SERVER_PACKET = 0,

	//=======================================
	// 하트비트 요청 packet
	// short Type
	//=======================================
	en_CHATTING_SERVER_PACKET_C2S_HEARTBEAT = 1,

	//=======================================
	// 하트비트 응답 packet
	// short Type
	//=======================================
	en_CHATTING_SERVER_PACKET_S2C_HEARTBEAT = 2,

	//=======================================
	// 채팅 요청 packet
	// short Type
	// string Message
	//=======================================
	en_CHATTING_SERVER_PACKET_C2S_CHAT = 3,

	//=======================================
	// 채팅 응답 packet
	// short Type
	// string Message
	//=======================================
	en_CHATTING_SERVER_PACKET_S2C_CHAT = 4,

	//========================================================
	// 채팅채널 생성 요청 packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_C2S_CREATE_CHATTING_CHANNEL = 5,

	//========================================================
	// 채팅채널 생성 응답 packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_S2C_CREATE_CHATTING_CHANNEL = 6,

	//======================================================
	// 채팅채널 입장 요청 packet
	// short Type
	// string ChannelName
	//======================================================
	en_CHATTING_SERVER_PACKET_C2S_JOIN_CHATTING_CHANNEL = 7,

	//======================================================
	// 채팅채널 입장 응답 packet
	// short Type
	// string ChannelName
	//======================================================
	en_CHATTING_SERVER_PACKET_S2C_JOIN_CHATTING_CHANNEL = 8,

	//========================================================
	// 채팅채널 나가기 요청 packet
	// short Type
	// string ChannelName
	//========================================================
	en_CHATTING_SERVER_PACKET_C2S_LEAVE_CHATTING_CHANNEL = 9,

	//========================================================
	// 채팅채널 나가기 응답 packet
	// short Type	
	//========================================================
	en_CHATTING_SERVER_PACKET_S2C_LEAVE_CHATTING_CHANNEL = 10,

	//================================================
	// 채팅채널 삭제 요청 packet
	// short Type
	// string ChannelName
	//================================================
	en_CHATTING_SERVER_PACKET_C2S_DELETE_CHANNEL = 11,

	//================================================
	// 채팅채널 삭제 응답 packet
	// short Type
	// string ChannelName
	//================================================
	en_CHATTING_SERVER_PACKET_S2C_DELETE_CHANNEL = 12,

	//=======================================================
	// 채팅채널 목록 요청 packet
	// short Type	
	//=======================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_LIST = 13,

	//=======================================================
	// 채팅채널 목록 응답 packet
	// short Type
	// string[] ChannelList
	//=======================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_LIST = 14,

	//==============================================================
	// 채팅채널 참여자 목록 요청 packet
	// short Type	
	//==============================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_MEMBER_LIST = 15,

	//==============================================================
	// 채팅채널 참여자 목록 응답 packet
	// short Type	
	// string[] MemberList
	//==============================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_MEMBER_LIST = 16,

	//=============================================================
	// 채팅채널 참여자 추가 요청 packet
	// short Type
	// string ChannelName
	// string MemberName
	//=============================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_ADD_MEMBER = 17,

	//=============================================================
	// 채팅채널 참여자 추가 응답 packet
	// short Type
	// string ChannelName
	// string MemberName
	//=============================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_ADD_MEMBER = 18,

	//================================================================
	// 채팅채널 참여자 제거 요청 packet
	// short Type
	// string ChannelName
	// string MemberName
	//================================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_REMOVE_MEMBER = 19,

	//================================================================
	// 채팅채널 참여자 제거 응답 packet
	// short Type
	// string ChannelName
	// string MemberName
	//================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_REMOVE_MEMBER = 20,

	//=======================================================================
	// 채팅채널 참여자 상태 변경 요청 packet
	// short Type
	// string ChannelName
	// string MemberName
	// int Status (0: offline, 1: online)
	//=======================================================================
	en_CHATTING_SERVER_PACKET_C2S_CHATTING_CHANNEL_MEMBER_STATUS_CHANGE = 21,

	//=======================================================================
	// 채팅채널 참여자 상태 변경 응답 packet
	// short Type
	// string ChannelName
	// string MemberName
	// int Status (0: offline, 1: online)
	//=======================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_MEMBER_STATUS_CHANGE = 22,

	//=================================================================
	// 채팅채널 메시지 알림 응답 packet
	// short Type
	// string ChannelName	
	// string Message
	//=================================================================
	en_CHATTING_SERVER_PACKET_S2C_CHATTING_CHANNEL_GLOBAL_MESSAGE = 23,
};