#include "Packet.h"
#include<cstring>
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

unsigned int Packet::MoveRearPosition(int size)
{
	return _rear;
}

unsigned int Packet::MoveFrontPosition(int size)
{
	return _front;
}

Packet& Packet::operator=(Packet& packet)
{
	memcpy(this, &packet, sizeof(Packet));
	return *(this);
}

Packet& Packet::operator<<(bool value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(bool));
	_rear += sizeof(bool);
	_useBufferSize += sizeof(bool);	
	return *(this);
}

Packet& Packet::operator<<(char value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(char));
	_rear += sizeof(char);
	_useBufferSize += sizeof(char);
	return *(this);
}

Packet& Packet::operator<<(unsigned char value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(unsigned char));
	_rear += sizeof(unsigned char);
	_useBufferSize += sizeof(unsigned char);
	return *(this);
}

Packet& Packet::operator<<(short value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(short));
	_rear += sizeof(short);
	_useBufferSize += sizeof(short);
	return *(this);
}

Packet& Packet::operator<<(unsigned short value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(unsigned short));
	_rear += sizeof(unsigned short);
	_useBufferSize += sizeof(unsigned short);
	return *(this);
}

Packet& Packet::operator<<(int value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(int));
	_rear += sizeof(int);
	_useBufferSize += sizeof(int);
	return *(this);
}

Packet& Packet::operator<<(unsigned int value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(unsigned int));
	_rear += sizeof(unsigned int);
	_useBufferSize += sizeof(unsigned int);
	return *(this);
}

Packet& Packet::operator<<(long value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(long));
	_rear += sizeof(long);
	_useBufferSize += sizeof(long);
	return *(this);
}

Packet& Packet::operator<<(unsigned long value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(unsigned long));
	_rear += sizeof(unsigned long);
	_useBufferSize += sizeof(unsigned long);
	return *(this);
}

Packet& Packet::operator<<(long long value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(long long));
	_rear += sizeof(long long);
	_useBufferSize += sizeof(long long);
	return *(this);
}

Packet& Packet::operator<<(unsigned long long value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(unsigned long long));
	_rear += sizeof(unsigned long long);
	_useBufferSize += sizeof(unsigned long long);
	return *(this);
}

Packet& Packet::operator<<(float value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(float));
	_rear += sizeof(float);
	_useBufferSize += sizeof(float);
	return *(this);
}

Packet& Packet::operator<<(double value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(double));
	_rear += sizeof(double);
	_useBufferSize += sizeof(double);
	return *(this);
}

int Packet::InsertData(char* src, int size)
{
	memcpy(&_packetBuffer[_rear], src, size);
	_rear += size;
	_useBufferSize += size;

	return size;
}

int Packet::InsertData(const char* src, int size)
{
	memcpy(&_packetBuffer[_rear], src, size);
	_rear += size;
	_useBufferSize += size;

	return size;
}

int Packet::InsertData(wchar_t* src, int size)
{
	memcpy(&_packetBuffer[_rear], src, size);
	_rear += size;
	_useBufferSize += size;

	return size;
}

int Packet::InsertData(const wchar_t* src, int size)
{
	memcpy(&_packetBuffer[_rear], src, size);
	_rear += size;
	_useBufferSize += size;

	return size;
}

Packet& Packet::operator>>(bool& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(bool));
	_front += sizeof(bool);
	_useBufferSize -= sizeof(bool);
	return *(this);
}

Packet& Packet::operator>>(char& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(char));
	_front += sizeof(char);
	_useBufferSize -= sizeof(char);
	return *(this);
}

Packet& Packet::operator>>(unsigned char& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(unsigned char));
	_front += sizeof(unsigned char);
	_useBufferSize -= sizeof(unsigned char);
	return *(this);
}

Packet& Packet::operator>>(short& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(short));
	_front += sizeof(short);
	_useBufferSize -= sizeof(short);
	return *(this);
}

Packet& Packet::operator>>(unsigned short& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(unsigned short));
	_front += sizeof(unsigned short);
	_useBufferSize -= sizeof(unsigned short);
	return *(this);
}

Packet& Packet::operator>>(int& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(int));
	_front += sizeof(int);
	_useBufferSize -= sizeof(int);
	return *(this);
}

Packet& Packet::operator>>(unsigned int& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(unsigned int));
	_front += sizeof(unsigned int);
	_useBufferSize -= sizeof(unsigned int);
	return *(this);
}

Packet& Packet::operator>>(long& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(long));
	_front += sizeof(long);
	_useBufferSize -= sizeof(long);
	return *(this);
}

Packet& Packet::operator>>(unsigned long& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(unsigned long));
	_front += sizeof(unsigned long);
	_useBufferSize -= sizeof(unsigned long);
	return *(this);
}

Packet& Packet::operator>>(long long& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(long long));
	_front += sizeof(long long);
	_useBufferSize -= sizeof(long long);
	return *(this);
}

Packet& Packet::operator>>(unsigned long long& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(unsigned long long));
	_front += sizeof(unsigned long long);
	_useBufferSize -= sizeof(unsigned long long);
	return *(this);
}

Packet& Packet::operator>>(float& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(float));
	_front += sizeof(float);
	_useBufferSize -= sizeof(float);
	return *(this);
}

Packet& Packet::operator>>(double& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(double));
	_front += sizeof(double);
	_useBufferSize -= sizeof(double);
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
	else if(size == 5)
	{
		memcpy(&_packetBuffer[_header], header, size);
		_useBufferSize += size;
	}
}

bool Packet::Encode(void)
{
	unsigned char checkSum = 0;
	unsigned long sum = 0;	

	// 헤더 준비
	EncodeHeader encodeHeader;
	encodeHeader.packetCode = 52;
	encodeHeader.packetLen = _useBufferSize;
	encodeHeader.randXORCode = Xoshiro256::GetInstance().RandNumber(1, 255);

	// 체크섬을 계산
	char* payload = &_packetBuffer[_front];
	for (int i = 0; i < _useBufferSize; i++)
	{
		sum += *payload;
		payload++;
	}

	// 더한 값을 256 으로 나머지 연산
	checkSum = sum % 256;

	// 체크섬 저장
	encodeHeader.checkSum = checkSum;

	SetHeader((char*)&encodeHeader, sizeof(EncodeHeader));

	int p1 = 0;
	int e1 = 0;

	// 체크섬 부터 암호화 시작
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

	// packetCode 검사
	if (decodeHeader->packetCode != 52)
	{
		return false;
	}

	// 길이 검사
	if (decodeHeader->packetLen != _useBufferSize - 5)
	{
		return false;
	}

	// 체크섬 부터 복호화
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

	// 복호화 후 체크섬을 구해서 검사
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