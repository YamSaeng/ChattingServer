#pragma once

#include"CoreNetwork.h"

class ChattingServer : public CoreNetwork
{
public:	

	ChattingServer();
	~ChattingServer();

	virtual void OnClientJoin(Session* newSession) override;
};