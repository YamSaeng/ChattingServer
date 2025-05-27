#include"pch.h"
#include"DummyClient.h"

DummyClient gDummyClient;

int main()
{
	bool isDummyCountSet = false;
	short dummyClientMenuSelectNum = 0;

	_setmode(_fileno(stdout), _O_U16TEXT);

	while (1)
	{
		wcout << L"채팅 더미 클라이언트 시작" << endl;
		wcout << L"1. 채팅 더미 개수 설정" << endl;
		wcout << L"2. 채팅 더미 서버 접속 시작" << endl;
		wcout << L"선택 : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			gDummyClient.DummyClientCountSet(dummyClientMenuSelectNum);
			break;
		case 2:
			if (isDummyCountSet == false)
			{
				wcout << L"채팅 더미 개수를 설정하고 접속하세요" << endl;
				wcout << L"====================================" << endl << endl;
			}
			break;
		default:
			break;
		}		

		Sleep(1000);
	}

	return 0;
}