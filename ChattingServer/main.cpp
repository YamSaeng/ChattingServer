#include"pch.h"
#include"ChattingServer.h"
#include"../Utils.h"
#include<rapidjson/document.h>

#include"DBConnectionPool.h"
#include"../Logger.h"

ChattingServer gChattingServer;

int main()
{
	ConfigureLogger(L"logs/log.log", LogLevel::DEBUG, true);

	_setmode(_fileno(stdout), _O_U8TEXT);

	string jsonStr = Utils::LoadFile(L"ServerInfo.json");

	rapidjson::Document doc;
	if (doc.Parse(jsonStr.c_str()).HasParseError()) {
		cerr << "JSON ÆÄ½Ì ½ÇÆÐ\n";
		return 1;
	}

	string ipAddress = doc["ipAddress"].GetString();
	int port = doc["port"].GetInt();

	gChattingServer.Start(Utils::Convert(ipAddress).c_str(), port);

	while (true)
	{
		LOG_DEBUG(L"======================");
		LOG_DEBUG(L"ChattingServer");
		LOG_DEBUGF(L"acceptTotal : [%d]", gChattingServer._acceptTotal);
		LOG_DEBUGF(L"acceptTPS : [%d]", gChattingServer._acceptTPS);
		LOG_DEBUGF(L"session : [%d]", gChattingServer.SessionCount());
		LOG_DEBUGF(L"recvTPS : [%d]", gChattingServer._recvPacketTPS);
		LOG_DEBUGF(L"sendTPS : [%d]", gChattingServer._sendPacketTPS);
		LOG_DEBUGF(L"updateTPS : [%d]", gChattingServer._updateTPS);
		LOG_DEBUGF(L"updateWakeCount : [%d]", gChattingServer._updateWakeCount);
		LOG_DEBUG(L"======================");		

		gChattingServer._acceptTPS = 0;
		gChattingServer._recvPacketTPS = 0;
		gChattingServer._sendPacketTPS = 0;
		gChattingServer._updateTPS = 0;

		Sleep(1000);

		system("cls");
	}

	return 0;
}