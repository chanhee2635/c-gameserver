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
	* @brief ��Ŷ ����� �Ϸ��ϰ� ���� ����� ������ ũ�⸦ Ȯ��
	* @param writeSize ������ ���ۿ� ����� ����Ʈ ũ��
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
public:
	SendBufferChunk();
	~SendBufferChunk();

	void			Reset();
	/* @brief ûũ ���� �޸𸮿��� allocSize��ŭ �Ҵ��� SendBuffer ��ȯ */
	SendBufferRef	Open(uint32 allocSize);
	/* @brief ���� ���� �Ҵ� ��û�� �ݰ�, ���� ��뷮�� Ȯ�� */
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
	* @brief ���ۿ� ����� �޸� ���۸� �Ҵ�
	* @param size ��û�� ���� ũ��
	* @return �Ҵ�� ������ �����ϴ� SendBufferRef
	* @details �Ź� �Ҵ����� �ʰ� TLS Chunk���� ���������� ������ �߶� ���� (Lock-Free)
	*/
	SendBufferRef		Open(uint32 size);

private:
	/*
	* @brief ���� Ǯ���� ��� ������ SendBufferChunk�� �ϳ� ����
	* @details Ǯ�� ����ִٸ� ���� ����, ���� Ƚ���� 0�� �Ǹ� �ڵ����� Ǯ�� �ݳ�
	*/
	SendBufferChunkRef	Pop();
	void				Push(SendBufferChunkRef buffer);

	static void			PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	Vector<SendBufferChunkRef> _sendBufferChunks;
};