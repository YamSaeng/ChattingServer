#include"pch.h"
#include"DummyClient.h"

DummyClient gDummyClient;

int main()
{
	bool isDummyCountSet = false;
	bool isDummyStartSet = false;
	short dummyClientMenuSelectNum = 0;
	int dummyClientCount = 0;

	_setmode(_fileno(stdout), _O_U16TEXT);

	while (1)
	{
		wcout << L"ä�� ���� Ŭ���̾�Ʈ ����" << endl;
		wcout << L"1. ä�� ���� ���� ����" << endl;
		wcout << L"2. ä�� ���� ���� ���� ����" << endl;
		wcout << L"���� : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			wcout << L"������ ���� ���� �Է� : ";
			cin >> dummyClientCount;			
			isDummyCountSet = true;

			wcout << endl;
			break;
		case 2:
			if (isDummyCountSet == false)
			{
				wcout << L"ä�� ���� ������ �����ϰ� �����ϼ���" << endl;
				wcout << L"====================================" << endl << endl;
				break;
			}

			gDummyClient.DummyClientStart(dummyClientCount);

			isDummyStartSet = true;
			break;
		default:
			break;
		}

		if (isDummyCountSet == true && isDummyStartSet == true)
		{
			break;
		}		
	}

	while (1)
	{
		Sleep(1000);
	}

	return 0;
}