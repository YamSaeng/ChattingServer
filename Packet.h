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

	static ObjectPool<Packet> _objectPool; // ObjectPool�� ����Ͽ� Packet�� �Ҵ�
	LONG* _retCount; // Packet�� ���ǰ� �ִ��� Ȯ���ϱ� ���� ī��Ʈ
	bool _isEncode;  // Packet�� �ѹ��� ���ڵ��ǵ��� �ϱ� ���� �÷���

	void Clear(void);

	unsigned int GetBufferSize(void);
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

	// ��� ����
	void SetHeader(char* header, char size);

	// Packet�� �Ҵ�
	static Packet* Alloc(); 
	// Packet�� �ݳ�
	void Free();
	// Packet ���� Ƚ�� ����
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