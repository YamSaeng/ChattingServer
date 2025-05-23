#pragma once

#include<stdlib.h>
#include<time.h>

#define PACKET_BUFFER_DEFAULT_SIZE	100000

// 패킷 클래스
class Packet
{
public:
#pragma pack(push,1)
	// 패킷 헤더
	struct EncodeHeader
	{
		unsigned char packetCode;

		unsigned short packetLen;

		unsigned char randXORCode;

		unsigned char checkSum;
	};
#pragma pack(pop)
protected:
	char _packetBuffer[PACKET_BUFFER_DEFAULT_SIZE];

	unsigned int _header;

	unsigned int _front;
	unsigned int _rear;

	unsigned int _bufferSize;
	unsigned int _useBufferSize;

	unsigned char _key;
public:
	Packet();
	~Packet();

	void Clear(void);

	unsigned int GetBufferSize(void);
	unsigned int GetUseBufferSize(void);
	unsigned int MoveRearPosition(int size);
	unsigned int MoveFrontPosition(int size);

	// 바이너리 데이터 넣기
	Packet& operator = (Packet& packet);
	Packet& operator << (bool value);
	Packet& operator << (char value);
	Packet& operator << (unsigned char value);	
	Packet& operator << (short value);
	Packet& operator << (unsigned short value);
	Packet& operator << (int value);
	Packet& operator << (unsigned int value);
	Packet& operator << (long value);
	Packet& operator << (unsigned long value);
	Packet& operator << (long long value);
	Packet& operator << (unsigned long long value);
	Packet& operator << (float value);
	Packet& operator << (double value);

	int InsertData(char* src, int size);
	int InsertData(const char* src, int size);

	int InsertData(wchar_t* src, int size);
	int InsertData(const wchar_t* src, int size);

	// 바이너리 데이터 빼기
	Packet& operator >> (bool& value);
	Packet& operator >> (char& value);
	Packet& operator >> (unsigned char& value);
	Packet& operator >> (short& value);
	Packet& operator >> (unsigned short& value);
	Packet& operator >> (int& value);
	Packet& operator >> (unsigned int& value);
	Packet& operator >> (long& value);
	Packet& operator >> (unsigned long& value);
	Packet& operator >> (long long& value);
	Packet& operator >> (unsigned long long& value);
	Packet& operator >> (float& value);
	Packet& operator >> (double& value);

	int GetData(char* dest, int size);
	int GetData(wchar_t* dest, int size);

	// 헤더 설정
	void SetHeader(char* header, char size);

	// 패킷 인코딩
	bool Encode(void);
	// 패킷 디코딩
	bool Decode(void);
};