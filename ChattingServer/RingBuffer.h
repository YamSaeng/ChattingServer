#pragma once

#define BUFFER_DEFAULT_SIZE	100001
#define BLANK				1

// ���� ť
class RingBuffer
{
private:
	char* _buffer;	
	int _front;
	int _rear;
	int _bufferMaxSize;

	bool Init(int bufferSize)
	{
		_front = 0;
		_rear = 0;		
		_bufferMaxSize = bufferSize;

		_buffer = new char[_bufferMaxSize];
		if (_buffer == nullptr)
		{
			std::cout << "RingBuffer �޸� �Ҵ� ����" << std::endl;
			return false;
		}
		memset(_buffer, 0, _bufferMaxSize);

		return true;
	}
public:
	RingBuffer(void)
	{
		if (!Init(BUFFER_DEFAULT_SIZE))
		{			
			CRASH("RingBuffer ���� ����");
		}		
	}

	RingBuffer(int bufferSize)
	{
		if (!Init(bufferSize))
		{
			CRASH("RingBuffer ���� ����");
		}
	}

	~RingBuffer()
	{
		delete[] _buffer;
	}

	int GetBufferSize(void)
	{
		return _bufferMaxSize;
	}

	//���� ����ϰ� �ִ� ���� ũ�� ��ȯ
	int GetUseSize(void)
	{
		int front = _front;
		int rear = _rear;
		int useSize = 0;
		
		if (rear >= front)
		{
			useSize = rear - front;
		}
		else
		{
			useSize = (_bufferMaxSize - front) + rear;
		}

		return useSize;
	}

	//���� �ִ� ���� ������
	int GetFreeSize(void)
	{
		int front = _front;
		int rear = _rear;
		int freeSize = 0;

		if (front > rear)
		{
			freeSize = front - rear - BLANK;
		}
		else
		{
			freeSize = (_bufferMaxSize - rear) + front - BLANK;
		}

		return freeSize;
	}

	// �ѹ��� enqueue �� �� �ִ� ������
	int GetDirectEnqueueSize(void)
	{
		int front = _front;
		int rear = _rear;
		int size = 0;

		if (front > rear)
		{
			size = front - rear - BLANK;
		}
		else
		{
			if (0 == front)
			{
				size = _bufferMaxSize - rear - BLANK;
			}
			else
			{
				size = _bufferMaxSize - rear;
			}
		}

		return size;
	}

	// �ѹ��� Dequeue �� �� �ִ� ������
	int GetDirectDequeueSize(void)
	{
		int tempFront = _front;
		int tempRear = _rear;
		int size = 0;

		if (tempRear >= tempFront)
		{
			size = tempRear - tempFront;
		}
		else
		{
			size = _bufferMaxSize - tempFront;
		}

		return size;
	}	

	// ������ �ֱ�
	int Enqueue(char* data, int size)
	{
		int directEnqSize = GetDirectEnqueueSize();
		int freeSize = GetFreeSize();

		if (size > freeSize)
		{
			size = freeSize;
		}

		if (size <= directEnqSize)
		{
			memcpy_s(&_buffer[_rear], size, data, size);
		}
		else
		{
			memcpy_s(&_buffer[_rear], directEnqSize, data, directEnqSize);
			memcpy_s(&_buffer[0], size - directEnqSize, data + directEnqSize, size - directEnqSize);
		}
		
		_rear = (_rear + size) % _bufferMaxSize;

		return size;
	}

	// ������ ����
	int Dequeue(char* dest, int size)
	{
		int directDeqSize = GetDirectDequeueSize();
		int useSize = GetUseSize();

		if (useSize < size)
		{
			size = useSize;
		}

		if (size <= directDeqSize)
		{
			memcpy_s(dest, size, &_buffer[_front], size);
		}
		else
		{
			memcpy_s(dest, directDeqSize, &_buffer[_front], directDeqSize);
			memcpy_s(dest + directDeqSize, size - directDeqSize, &_buffer[0], size - directDeqSize);
		}

		_front = (_front + size) % _bufferMaxSize;

		return size;
	}

	// �����Ͱ� �ִ��� Ȯ��
	int Peek(char* dest, int size)
	{
		int directDeqSize = GetDirectDequeueSize();
		int useSize = GetUseSize();

		if (useSize < size)
		{
			size = useSize;
		}

		if (size <= directDeqSize)
		{
			memcpy_s(dest, size, &_buffer[_front], size);
		}
		else
		{
			memcpy_s(dest, directDeqSize, &_buffer[_front], directDeqSize);
			memcpy_s(dest + directDeqSize, size - directDeqSize, &_buffer[0], size - directDeqSize);
		}

		return size;
	}	

	// rear �����̱�
	int MoveRear(int size)
	{
		int freeSize = GetFreeSize();
		if (freeSize < size)
		{
			size = freeSize;
		}

		_rear = (_rear + size) % _bufferMaxSize;

		return size;
	}

	// front �����̱�
	int MoveFront(int size)
	{
		int useSize = GetUseSize();
		if (useSize < size)
		{
			size = useSize;
		}

		_front = (_front + size) % _bufferMaxSize;

		return size;
	}

	// ringbuffer �ʱ�ȭ
	void ClearBuffer(void)
	{
		_front = _rear = 0;
	}

	// ����մ��� Ȯ��
	bool IsEmpty(void)
	{
		return (_front == _rear);
	}

	// front ��ġ ��ȯ
	char* GetFrontBufferPtr(void)
	{
		return &_buffer[_front];
	}

	// rear ��ġ ��ȯ
	char* GetRearBufferPtr(void)
	{
		return &_buffer[_rear];
	}

	// buffer �Ǿ� ��ȯ
	char* GetBufferPtr(void)
	{
		return &_buffer[0];
	}
};