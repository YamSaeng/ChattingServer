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
		cout << "ä�� ���� Ŭ���̾�Ʈ ����" << endl;
		cout << "1. ä�� ���� ����" << endl;
		cout << "2. ä�� ���� ���� ���� ����" << endl;
		cout << "���� : ";
		cin >> dummyClientMenuSelectNum;

		switch (dummyClientMenuSelectNum)
		{
		case 1:
			if (dummyClientCount > 0)
			{
				cout << "���� ���� �Ϸ� 2���� �Է��� ���� ���� ����" << endl << endl;
				continue;
			}

			cout << "������ ���� ���� �Է� : ";
			cin >> dummyClientCount;
			isDummyCountSet = true;

			cout << "���� ������� ���� (1: �������, 0:�����������) : ";
			cin >> isDummyDisconnect;

			if (isDummyDisconnect == true)
			{
				cout << "���� ���� ���� Ȯ�� ���� (1 ~ 80%) : ";
				cin >> probilitydummyDisconnect;				

				cout << "���� ������ ���� (1: ������ �õ� O, 0: ������ �õ� X ) : ";
				cin >> isDummyReconnectTry;				
			}			

			cout << endl;

			system("cls");
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
			cout << "���� Ŭ���̾�Ʈ " << "[" << dummyClientCount << "] ��" << " ����" << endl;

			dummyClientManager.Start(dummyClientCount, isDummyDisconnect, isDummyReconnectTry, probilitydummyDisconnect, L"127.0.0.1", 8888);
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