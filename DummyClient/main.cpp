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
		wcout << L"ä�� ���� Ŭ���̾�Ʈ ����" << endl;
		wcout << L"1. ä�� ���� ���� ����" << endl;
		wcout << L"2. ä�� ���� ���� ���� ����" << endl;
		wcout << L"���� : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			gDummyClient.DummyClientCountSet(dummyClientMenuSelectNum);
			break;
		case 2:
			if (isDummyCountSet == false)
			{
				wcout << L"ä�� ���� ������ �����ϰ� �����ϼ���" << endl;
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