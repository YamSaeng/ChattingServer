#include"pch.h"
#include"ChattingServer.h"
#include"../Utils.h"
#include<rapidjson/document.h>

ChattingServer gChattingServer;

int main()
{	
	std::string jsonStr = Utils::LoadFile(L"ServerInfo.json");

	rapidjson::Document doc;
	if (doc.Parse(jsonStr.c_str()).HasParseError()) {
		std::cerr << "JSON ÆÄ½Ì ½ÇÆÐ\n";
		return 1;
	}

	std::string ipAddress = doc["ipAddress"].GetString();
	int port = doc["port"].GetInt();

	gChattingServer.Start(Utils::Convert(ipAddress).c_str(), port);
		
	while (true)
	{
		cout << "===================" << endl << endl;
		cout << "ChattingServer" << endl << endl;
		cout << "acceptTotal : [ " << gChattingServer._acceptTotal << " ]" << endl;
		cout << "acceptTPS : [ " << gChattingServer._acceptTPS << " ]" << endl;
		cout << "session : [ " << gChattingServer.SessionCount() << " ]" << endl;
		cout << "recvTPS : [ " << gChattingServer._recvPacketTPS << " ]" << endl;
		cout << "sendTPS : [ " << gChattingServer._sendPacketTPS << " ]" << endl;
		cout << "===================";

		gChattingServer._acceptTPS = 0;
		gChattingServer._recvPacketTPS = 0;
		gChattingServer._sendPacketTPS = 0;

		Sleep(1000);

		system("cls");
	}

	return 0;
}