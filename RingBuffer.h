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

// 원형 큐
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

	//현재 사용하고 있는 버퍼 크기 반환
	int GetUseSize(void);

	//남아 있는 공간 사이즈
	int GetFreeSize(void);

	// 한번에 enqueue 할 수 있는 사이즈
	int GetDirectEnqueueSize(void);

	// 한번에 Dequeue 할 수 있는 사이즈
	int GetDirectDequeueSize(void);

	// 데이터 넣기
	int Enqueue(char* data, int size);

	// 데이터 빼기
	int Dequeue(char* dest, int size);

	// 데이터가 있는지 확인
	int Peek(char* dest, int size);

	// rear 움직이기
	int MoveRear(int size);

	// front 움직이기
	int MoveFront(int size);

	// ringbuffer 초기화
	void ClearBuffer(void);

	// 비어잇는지 확인
	bool IsEmpty(void);

	// front 위치 반환
	char* GetFrontBufferPtr(void);

	// rear 위치 반환
	char* GetRearBufferPtr(void);

	// buffer 맨앞 반환
	char* GetBufferPtr(void);
};