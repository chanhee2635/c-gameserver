#pragma once

class SendBufferChunk;

/*--------------
	SendBuffer
---------------*/

class SendBuffer
{
public:
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize);
	~SendBuffer();

	BYTE*			Buffer() { return _buffer; }
	uint32			AllocSize() { return _allocSize; }
	uint32			WriteSize() { return _writeSize; }
	/*
	* @brief 패킷 기록을 완료하고 실제 사용한 데이터 크기를 확정
	* @param writeSize 실제로 버퍼에 기록한 바이트 크기
	*/
	void			Close(uint32 writeSize);

private:
	BYTE*				_buffer;
	uint32				_allocSize = 0;
	uint32				_writeSize = 0;
	SendBufferChunkRef	_owner;
};

/*--------------------
	SendBufferChunk
---------------------*/

class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum
	{
		SEND_BUFFER_CHUNK_SIZE = 65536
	};

public:
	SendBufferChunk();
	~SendBufferChunk();

	void			Reset();
	/* @brief 청크 내부 메모리에서 allocSize만큼 할당해 SendBuffer 반환 */
	SendBufferRef	Open(uint32 allocSize);
	/* @brief 현재 열린 할당 요청을 닫고, 실제 사용량을 확정 */
	void			Close(uint32 writeSize);

	bool			IsOpen() { return _open; }
	BYTE*			Buffer() { return &_buffer[_usedSize]; }
	uint32			FreeSize() { return static_cast<uint32>(_buffer.size() - _usedSize); }

private:
	Array<BYTE, SEND_BUFFER_CHUNK_SIZE>		_buffer = {};
	bool									_open = false;
	uint32									_usedSize = 0;
};

/*---------------------
	SendBufferManager
----------------------*/

class SendBufferManager
{
public:
	/*
	* @brief 전송에 사용할 메모리 버퍼를 할당
	* @param size 요청할 버퍼 크기
	* @return 할당된 공간을 관리하는 SendBufferRef
	* @details 매번 할당하지 않고 TLS Chunk에서 순차적으로 공간을 잘라서 제공 (Lock-Free)
	*/
	SendBufferRef		Open(uint32 size);

private:
	/*
	* @brief 전역 풀에서 사용 가능한 SendBufferChunk를 하나 꺼냄
	* @details 풀이 비어있다면 새로 생성, 참조 횟수가 0이 되면 자동으로 풀에 반납
	*/
	SendBufferChunkRef	Pop();
	void				Push(SendBufferChunkRef buffer);

	static void			PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	Vector<SendBufferChunkRef> _sendBufferChunks;
};