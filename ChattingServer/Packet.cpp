#include "Packet.h"
#include<cstring>

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

}

bool Packet::Decode(void)
{

}