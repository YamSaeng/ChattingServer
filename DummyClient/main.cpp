#include"pch.h"
#include"DummyClientManager.h"

int main()
{
	DummyClientManager dummyClientManager;

	bool isDummyCountSet = false;
	bool isDummyStartSet = false;
	short dummyClientMenuSelectNum = 0;
	int dummyClientCount = 0;	

	while (1)
	{
		cout << "채팅 더미 클라이언트 시작" << endl;
		cout << "1. 채팅 더미 개수 설정" << endl;
		cout << "2. 채팅 더미 서버 접속 시작" << endl;
		cout << "선택 : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			cout << "생성할 더미 개수 입력 : ";
			cin >> dummyClientCount;			
			isDummyCountSet = true;

			cout << endl;
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
			cout << "더미 클라이언트 "<<"["<<dummyClientCount<<"] 개"<<" 시작" << endl;

			dummyClientManager.Start(dummyClientCount, L"127.0.0.1", 8888);
			break;
		}		
	}

	system("cls");

	while (1)
	{
		cout << "===================" << endl << endl;
		cout << "DummyClient" << endl << endl;				
		cout << "sendTPS : [ " << dummyClientManager._sendPacketTPS << " ]" << endl;
		cout << "===================";

		dummyClientManager._sendPacketTPS = 0;

		Sleep(1000);

		system("cls");
	}

	return 0;
}