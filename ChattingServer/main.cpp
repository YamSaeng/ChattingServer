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

   	_setmode(_fileno(stdout), _O_U16TEXT);	

	while (true)
	{
		wcout << L"===================" << endl << endl;
		wcout << L"ChattingServer" << endl << endl;
		wcout << L"acceptTotal : [ " << gChattingServer._acceptTotal << " ]" << endl;
		wcout << L"acceptTPS : [ " << gChattingServer._acceptTPS << " ]" << endl;
		wcout << L"===================";

		Sleep(1000);

		system("cls");
	}

	return 0;
}