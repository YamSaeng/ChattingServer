#pragma once
#include"ObjectPool.h"

#ifdef PACKET
#define PACKET_DLL __declspec(dllexport)
#else
#define PACKET_DLL __declspec(dllimport)
#endif

#define PACKET_BUFFER_DEFAULT_SIZE	100000

// ��Ŷ Ŭ����
class PACKET_DLL Packet
{
public:
#pragma pack(push,1)
	// ��Ŷ ���
	struct EncodeHeader
	{
		unsigned char packetCode; // packet Code 
		unsigned short packetLen; // packet ����
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

	static ObjectPool<Packet> _objectPool; // Packet Ǯ
	LONG* _retCount; // packet ���� Ƚ��
	bool _isEncode; // packet�� Encoding �ߴ��� ����

	void Clear(void);

	unsigned int GetBufferSize(void); // packet �⺻ ũ�� ��ȯ
	unsigned int GetUseBufferSize(void);

	char* GetBufferPtr(void); 
	char* GetHeaderBufferPtr(void);
	char* GetFrontBufferPtr(void);
	char* GetRearBufferPtr(void);

	void MoveRearPosition(int size);
	void MoveFrontPosition(int size);

	// ���̳ʸ� ������ �ֱ�
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

	int InsertData(char* Src, int Size);

	// ��� ����
	void SetHeader(char* header, char size);

	// packet �Ҵ�
	static Packet* Alloc();
	void Free();
	void AddRetCount();

	// ��Ŷ ���ڵ�
	bool Encode(void);
	// ��Ŷ ���ڵ�
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