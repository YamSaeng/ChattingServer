#pragma once

#include <iostream>

using namespace std;

#define BUFFER_DEFAULT_SIZE	100001
#define BLANK				1

#ifdef RINGBUFFER
#define RINGBUFFER_DLL __declspec(dllexport)
#else
#define RINGBUFFER_DLL __declspec(dllimport)
#endif

// ���� ť
class RINGBUFFER_DLL RingBuffer
{
private:
	char* _buffer;
	int _front;
	int _rear;
	int _bufferMaxSize;

	bool Init(int bufferSize);	
public:
	RingBuffer(void);
	RingBuffer(int bufferSize);
	~RingBuffer();

	int GetBufferSize(void);

	//���� ����ϰ� �ִ� ���� ũ�� ��ȯ
	int GetUseSize(void);

	//���� �ִ� ���� ������
	int GetFreeSize(void);

	// �ѹ��� enqueue �� �� �ִ� ������
	int GetDirectEnqueueSize(void);

	// �ѹ��� Dequeue �� �� �ִ� ������
	int GetDirectDequeueSize(void);

	// ������ �ֱ�
	int Enqueue(char* data, int size);

	// ������ ����
	int Dequeue(char* dest, int size);

	// �����Ͱ� �ִ��� Ȯ��
	int Peek(char* dest, int size);

	// rear �����̱�
	int MoveRear(int size);

	// front �����̱�
	int MoveFront(int size);

	// ringbuffer �ʱ�ȭ
	void ClearBuffer(void);

	// ����մ��� Ȯ��
	bool IsEmpty(void);

	// front ��ġ ��ȯ
	char* GetFrontBufferPtr(void);

	// rear ��ġ ��ȯ
	char* GetRearBufferPtr(void);

	// buffer �Ǿ� ��ȯ
	char* GetBufferPtr(void);
};