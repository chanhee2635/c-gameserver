#include "pch.h"
#include "SendBuffer.h"

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
	ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize()) return nullptr;

	_open = true;
	// SendBuffer 객체 생성 및 반환
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
	// 스레드 전용 Chunk가 없는 경우 초기화
	if (LSendBufferChunk == nullptr)
	{
		// 전역 Pool에서 큰 메모리 덩어리 하나를 가져옴
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	// 중복 호출 체크 (Close 를 하지 않은 경우)
	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	// Chunk에 남은 공간이 요청한 크기보다 작은 경우 
	if (LSendBufferChunk->FreeSize() < size)
	{
		// 기존 Chunk 교체
		// (기존 Chunk의 RefCount가 0이 되면 자동으로 PushGlobal 호출되어 Pool에 반납)
		LSendBufferChunk = Pop();
		LSendBufferChunk->Reset();
	}

	// Chunk에서 요청한 크기만큼 공간을 예약하여 SendBuffer 객체 반환
	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	{
		// 전역 풀에 접근하기 위한 LOCK
		WRITE_LOCK;
		// 풀에 재사용 가능한 청크가 남아있는지 확인
		if (_sendBufferChunks.empty() == false)
		{
			// LIFO 방식 사용으로 성능 최적화 (캐시에 메모리 주소가 남아있을 가능성↑)
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	// 풀이 비어있다면 새로운 청크를 생성 (RefCount가 0이 되면 PushGlobal 함수를 호출하여 풀에 반납)
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

