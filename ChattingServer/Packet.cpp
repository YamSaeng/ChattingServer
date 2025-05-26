#include"pch.h"
#include"Packet.h"
#include"Xoshiro.h"

Packet::Packet()
{
	memset(_packetBuffer, 0, sizeof(_packetBuffer));
	_header = 0;

	_front = 5;
	_rear = 5;

	_bufferSize = PACKET_BUFFER_DEFAULT_SIZE;
	_useBufferSize = 0;

	_key = 50;
}

Packet::~Packet()
{
}

void Packet::Clear(void)
{
	_header = 0;
	_front = 5;
	_rear = 5;

	_useBufferSize = 0;
}

unsigned int Packet::GetBufferSize(void)
{
	return _bufferSize;
}

unsigned int Packet::GetUseBufferSize(void)
{
	return _useBufferSize;
}

void Packet::MoveRearPosition(int size)
{
	_rear += size;
	_useBufferSize += size;
}

void Packet::MoveFrontPosition(int size)
{
	_front += size;
	_useBufferSize += size;
}

Packet& Packet::operator=(Packet& packet)
{
	memcpy(this, &packet, sizeof(Packet));
	return *(this);
}

int Packet::GetData(char* dest, int size)
{
	memcpy(dest, &_packetBuffer[_front], size);
	_front += size;
	_useBufferSize -= size;
	return size;
}

int Packet::GetData(wchar_t* dest, int size)
{
	memcpy(dest, &_packetBuffer[_front], size);
	_front += size;
	_useBufferSize -= size;
	return size;
}

void Packet::SetHeader(char* header, char size)
{
	if (size == 2)
	{
		_header = 3;
		memcpy(&_packetBuffer[_header], header, size);
		_useBufferSize += size;
	}
	else if (size == 5)
	{
		memcpy(&_packetBuffer[_header], header, size);
		_useBufferSize += size;
	}
}

bool Packet::Encode(void)
{
	unsigned char checkSum = 0;
	unsigned long sum = 0;

	// ��� �غ�
	EncodeHeader encodeHeader;
	encodeHeader.packetCode = 52;
	encodeHeader.packetLen = _useBufferSize;
	encodeHeader.randXORCode = Xoshiro256::GetInstance().RandNumber(1, 255);

	// üũ���� ���
	char* payload = &_packetBuffer[_front];
	for (int i = 0; i < _useBufferSize; i++)
	{
		sum += *payload;
		payload++;
	}

	// ���� ���� 256 ���� ������ ����
	checkSum = sum % 256;

	// üũ�� ����
	encodeHeader.checkSum = checkSum;

	SetHeader((char*)&encodeHeader, sizeof(EncodeHeader));

	int p1 = 0;
	int e1 = 0;

	// üũ�� ���� ��ȣȭ ����
	char* movePoint = &_packetBuffer[_front - 1];

	for (int i = 0; i < encodeHeader.packetLen + 1; i++)
	{
		p1 = *movePoint ^ (p1 + encodeHeader.randXORCode + i + 1);
		e1 = p1 ^ (e1 + _key + i + 1);
		*movePoint = e1;
		movePoint++;
	}

	return true;
}

bool Packet::Decode(void)
{
	unsigned long sum = 0;
	int p1 = 0;
	int e1 = 0;

	EncodeHeader* decodeHeader = (EncodeHeader*)_packetBuffer;

	// packetCode �˻�
	if (decodeHeader->packetCode != 52)
	{
		return false;
	}

	// ���� �˻�
	if (decodeHeader->packetLen != _useBufferSize - 5)
	{
		return false;
	}

	// üũ�� ���� ��ȣȭ
	char* movePoint = &_packetBuffer[_front - 1];
	int decodePoint = 0;

	for (int i = 0; i < decodeHeader->packetLen + 1; i++)
	{
		p1 = *movePoint ^ (decodePoint + _key + i + 1);
		decodePoint = *movePoint;
		e1 = p1 ^ (e1 + decodeHeader->randXORCode + i + 1);
		*movePoint = e1;
		e1 = p1;
		movePoint++;
	}

	// ��ȣȭ �� üũ���� ���ؼ� �˻�
	unsigned char checkSum = 0;
	char* payload = &_packetBuffer[_front];

	for (int i = 0; i < decodeHeader->packetLen; i++)
	{
		sum += *payload;
		payload++;
	}

	checkSum = sum % 256;

	if (decodeHeader->checkSum != checkSum)
	{
		return false;
	}

	return true;
}