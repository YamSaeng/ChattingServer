#pragma once
#include"ObjectPool.h"

#ifdef PACKET
#define PACKET_DLL __declspec(dllexport)
#else
#define PACKET_DLL __declspec(dllimport)
#endif

#define PACKET_BUFFER_DEFAULT_SIZE	100000

// 패킷 클래스
class PACKET_DLL Packet
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

	static ObjectPool<Packet> _objectPool; // ObjectPool을 사용하여 Packet을 할당
	LONG* _retCount; // Packet이 사용되고 있는지 확인하기 위한 카운트
	bool _isEncode;  // Packet이 한번만 인코딩되도록 하기 위한 플래그

	void Clear(void);

	unsigned int GetBufferSize(void);
	unsigned int GetUseBufferSize(void);

	char* GetBufferPtr(void);
	char* GetHeaderBufferPtr(void);
	char* GetFrontBufferPtr(void);
	char* GetRearBufferPtr(void);

	void MoveRearPosition(int size);
	void MoveFrontPosition(int size);

	// 바이너리 데이터 넣기
	Packet& operator = (Packet& packet);

	template<typename T>
	Packet& operator<<(T& value);

	template<typename T>
	Packet& operator<<(const std::pair<T, int>& value);

	template<typename T>
	Packet& operator>>(T& value);

	template<typename T>
	Packet& operator>>(const std::pair<T, int>& value);

	int GetData(char* dest, int size);
	int GetData(wchar_t* dest, int size);

	// 헤더 설정
	void SetHeader(char* header, char size);

	// Packet을 할당
	static Packet* Alloc(); 
	// Packet을 반납
	void Free();
	// Packet 참조 횟수 증가
	void AddRetCount();

	// 패킷 인코딩
	bool Encode(void);
	// 패킷 디코딩
	bool Decode(void);
};

template<typename T>
Packet& Packet::operator<<(T& value)
{
	memcpy(&_packetBuffer[_rear], &value, sizeof(T));
	_rear += sizeof(T);
	_useBufferSize += sizeof(T);

	return *(this);
}

template<typename T>
Packet& Packet::operator<<(const std::pair<T, int>& value)
{
	*this << value.second;
	memcpy(&_packetBuffer[_rear], value.first, value.second);
	_rear += value.second;
	_useBufferSize += value.second;

	return *(this);
}

template<typename T>
Packet& Packet::operator>>(T& value)
{
	memcpy(&value, &_packetBuffer[_front], sizeof(T));
	_front += sizeof(T);
	_useBufferSize -= sizeof(T);

	return *(this);
}

template<typename T>
Packet& Packet::operator>>(const std::pair<T, int>& value)
{
	memcpy(value.first, &_packetBuffer[_front], value.second);
	_front += value.second;
	_useBufferSize -= value.second;

	return *(this);
}