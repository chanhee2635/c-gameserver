#include "pch.h"
#include "SendBuffer.h"
#include "GameMetrics.h"

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{

}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*--------------------
	SendBufferChunk
---------------------*/

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
	ASSERT_CRASH(allocSize <= Config::Buffer::SEND_BUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize()) return nullptr;

	_open = true;
	// SendBuffer ��ü ���� �� ��ȯ
	return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_open == true);
	_open = false;
	_usedSize += writeSize;
}

/*---------------------
	SendBufferManager
----------------------*/

SendBufferRef SendBufferManager::Open(uint32 size)
{
	// ������ ���� Chunk�� ���� ��� �ʱ�ȭ
	if (LSendBufferChunk == nullptr)
	{
		// ���� Pool���� ū �޸� ��� �ϳ��� ������
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	// �ߺ� ȣ�� üũ (Close �� ���� ���� ���)
	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	// Chunk�� ���� ������ ��û�� ũ�⺸�� ���� ��� 
	if (LSendBufferChunk->FreeSize() < size)
	{
		// ���� Chunk ��ü
		// (���� Chunk�� RefCount�� 0�� �Ǹ� �ڵ����� PushGlobal ȣ��Ǿ� Pool�� �ݳ�)
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	// Chunk���� ��û�� ũ�⸸ŭ ������ �����Ͽ� SendBuffer ��ü ��ȯ
	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	{
		// ���� Ǯ�� �����ϱ� ���� LOCK
		WRITE_LOCK;
		// Ǯ�� ���� ������ ûũ�� �����ִ��� Ȯ��
		if (_sendBufferChunks.empty() == false)
		{
			GMetrics.sendBufferChunkReuse.fetch_add(1);

			// LIFO ��� ������� ���� ����ȭ (ĳ�ÿ� �޸� �ּҰ� �������� ���ɼ���)
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	GMetrics.sendBufferChunkAlloc.fetch_add(1);

	// Ǯ�� ����ִٸ� ���ο� ûũ�� ���� (RefCount�� 0�� �Ǹ� PushGlobal �Լ��� ȣ���Ͽ� Ǯ�� �ݳ�)
	return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
	WRITE_LOCK;
	_sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}

