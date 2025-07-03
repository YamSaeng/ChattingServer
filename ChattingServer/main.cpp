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
		cerr << "JSON �Ľ� ����\n";
		return 1;
	}

	string ipAddress = doc["ipAddress"].GetString();
	int port = doc["port"].GetInt();

	gChattingServer.Start(Utils::Convert(ipAddress).c_str(), port);

	while (true)
	{
		LOG_INFO(L"======================");
		LOG_INFO(L"ChattingServer");
		LOG_INFOF(L"acceptTotal : [%d]", gChattingServer._acceptTotal);
		LOG_INFOF(L"acceptTPS : [%d]", gChattingServer._acceptTPS);
		LOG_INFOF(L"session : [%d]", gChattingServer.SessionCount());
		LOG_INFOF(L"recvTPS : [%d]", gChattingServer._recvPacketTPS);
		LOG_INFOF(L"sendTPS : [%d]", gChattingServer._sendPacketTPS);
		LOG_INFOF(L"updateTPS : [%d]", gChattingServer._updateTPS);
		LOG_INFOF(L"updateWakeCount : [%d]", gChattingServer._updateWakeCount);
		LOG_INFO(L"======================");

		gChattingServer._acceptTPS = 0;
		gChattingServer._recvPacketTPS = 0;
		gChattingServer._sendPacketTPS = 0;
		gChattingServer._updateTPS = 0;

		Sleep(1000);

		system("cls");
	}

	return 0;
}