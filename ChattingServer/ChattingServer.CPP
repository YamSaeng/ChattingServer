#include "pch.h"
#include "ChattingServer.h"
#include "../protocol.h"

ChattingServer::ChattingServer()
{
	wstring host = L"tcp://127.0.0.1:3306";
	wstring user = L"root";
	wstring password = L"02415325";
	wstring dbName = L"chattingdb";

	_dbPool.Initialize(
		host,
		user,
		password,
		dbName,
		3, 10, 300
	);

	_hUpdateWakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	for (int i = 0; i < 5; i++)
	{
		_hUpdateThreads.push_back((HANDLE)_beginthreadex(NULL, 0, UpdateThreadProc, this, 0, NULL));
	}

	packetProc[en_CHATTING_SERVER_PACKET_C2S_CHAT] = &ChattingServer::PacketProcReqChat;

	_updateWakeCount = 0;
	_updateTPS = 0;

	_jobPool = new ObjectPool<Job>();

	InitializeCriticalSection(&_jobQueueLock);
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
		if (session->clientSocket != INVALID_SOCKET)
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

unsigned __stdcall ChattingServer::UpdateThreadProc(void* argument)
{
	ChattingServer* instance = (ChattingServer*)argument;
	while (1)
	{
		WaitForSingleObject(instance->_hUpdateWakeEvent, INFINITE);

		InterlockedIncrement64(&instance->_updateWakeCount);

		EnterCriticalSection(&instance->_jobQueueLock);
		while (!instance->_jobQueue.empty())
		{
			Job* job = instance->_jobQueue.front();
			instance->_jobQueue.pop();

			LeaveCriticalSection(&instance->_jobQueueLock);

			short protocol;
			*job->packet >> protocol;

			(instance->*(instance->packetProc[protocol]))(job->sessionId, job->packet);

			job->packet->Free();

			EnterCriticalSection(&instance->_jobQueueLock);
			instance->_jobPool->Free(job);

			InterlockedIncrement64(&instance->_updateTPS);
		}
		LeaveCriticalSection(&instance->_jobQueueLock);
	}
}

void ChattingServer::OnClientJoin(Session* newSession)
{
}

void ChattingServer::OnRecv(__int64 sessionId, Packet* packet)
{
	Job* job = _jobPool->Alloc();
	job->sessionId = sessionId;

	Packet* jobPacket = Packet::Alloc();
	if (jobPacket == nullptr)
	{
		_jobPool->Free(job);
		CRASH("JobPacket nullptr");
	}

	jobPacket->Clear();
	jobPacket->SetHeader(packet->GetBufferPtr(), sizeof(Packet::EncodeHeader));
	jobPacket->InsertData(packet->GetFrontBufferPtr(), packet->GetUseBufferSize() - sizeof(Packet::EncodeHeader));

	job->packet = jobPacket;

	EnterCriticalSection(&_jobQueueLock);
	_jobQueue.push(job);
	LeaveCriticalSection(&_jobQueueLock);

	SetEvent(_hUpdateWakeEvent);
}

void ChattingServer::OnClientLeave(Session* leaveSession)
{
}

