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
		cout << "ä�� ���� Ŭ���̾�Ʈ ����" << endl;
		cout << "1. ä�� ���� ���� ����" << endl;
		cout << "2. ä�� ���� ���� ���� ����" << endl;
		cout << "���� : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			cout << "������ ���� ���� �Է� : ";
			cin >> dummyClientCount;			
			isDummyCountSet = true;

			cout << endl;
			break;
		case 2:
			if (isDummyCountSet == false)
			{
				cout << "ä�� ���� ������ �����ϰ� �����ϼ���" << endl;
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
			cout << "���� ���� �Ϸ�" << endl;
			cout << "���� Ŭ���̾�Ʈ "<<"["<<dummyClientCount<<"] ��"<<" ����" << endl;

			dummyClientManager.Start(dummyClientCount, L"127.0.0.1", 8888);
			break;
		}		
	}

	while (1)
	{
		Sleep(1000);
	}

	return 0;
}