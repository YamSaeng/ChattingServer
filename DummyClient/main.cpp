#include"pch.h"
#include"DummyClientManager.h"

int main()
{
	DummyClientManager dummyClientManager;

	bool isDummyCountSet = false;
	bool isDummyStartSet = false;
	bool isDummyReconnectTry = false;
	bool isDummyDisconnect = false;
	int probilitydummyDisconnect = 0;
	short dummyClientMenuSelectNum = 0;
	int dummyClientCount = 0;

	while (1)
	{
		cout << "채팅 더미 클라이언트 시작" << endl;
		cout << "1. 채팅 더미 설정" << endl;
		cout << "2. 채팅 더미 서버 접속 시작" << endl;
		cout << "선택 : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			if (dummyClientCount > 0)
			{
				cout << "더미 생성 완료 2번을 입력해 서버 접속 시작" << endl << endl;
				continue;
			}

			cout << "생성할 더미 개수 입력 : ";
			cin >> dummyClientCount;
			isDummyCountSet = true;

			cout << "더미 연결끊김 설정 (1: 연결끊기, 0:연결끊지않음) : ";
			cin >> isDummyDisconnect;

			if (isDummyDisconnect == true)
			{
				cout << "더미 연결 끊김 확률 지정 (1 ~ 80%) : ";
				cin >> probilitydummyDisconnect;				

				cout << "더미 재접속 설정 (1: 재접속 시도 O, 0: 재접속 시도 X ) : ";
				cin >> isDummyReconnectTry;				
			}			

			cout << endl;

			system("cls");
			break;
		case 2:
			if (isDummyCountSet == false)
			{
				cout << "채팅 더미 개수를 설정하고 접속하세요" << endl;
				cout << "====================================" << endl << endl;
				break;
			}

			isDummyStartSet = true;
			break;
		default:
			break;
		}

		if (isDummyCountSet == true && isDummyStartSet == true)
		{
			cout << "더미 설정 완료" << endl;
			cout << "더미 클라이언트 " << "[" << dummyClientCount << "] 개" << " 시작" << endl;

			dummyClientManager.Start(dummyClientCount, isDummyDisconnect, isDummyReconnectTry, probilitydummyDisconnect, L"127.0.0.1", 8888);
			break;
		}
	}

	system("cls");

	while (1)
	{
		cout << "===================" << endl << endl;
		cout << "DummyClient" << endl << endl;
		cout << "DummyClientCount : [ " << dummyClientManager._dummyClientCount << " ]" << endl;
		cout << "sendTPS : [ " << dummyClientManager._sendPacketTPS << " ]" << endl;		
		cout << "===================";

		dummyClientManager._sendPacketTPS = 0;

		Sleep(1000);

		system("cls");
	}

	return 0;
}