#include"pch.h"

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

		if (dummyClientMenuSelectNum == 2)
		{
			if (isDummyCountSet == false)
			{
				wcout << L"ä�� ���� ������ �����ϰ� �����ϼ���" << endl;
				wcout << L"====================================" << endl << endl;
			}
		}

		Sleep(1000);
	}

	return 0;
}