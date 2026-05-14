#pragma once

class SendBufferChunk;

class SendBuffer
{
public:
    SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize);

    BYTE* GetBuffer() { return _buffer; }
    uint32 GetWriteSize() const { return _writeSize; }
    void   Close(uint32 writeSize);

private:
    SendBufferChunkRef _owner;
    BYTE* _buffer = nullptr;
    uint32 _allocSize = 0;
    uint32 _writeSize = 0;
};

class SendBufferChunk   
{
    friend class SendBuffer;
    friend struct SendBufferChunkNode;

public:
    SendBufferChunk() = default;

    void          Reset();
    SendBufferRef Open(uint32 allocSize, SendBufferChunkRef self);
    bool          IsOpen()      const { return _open; }
    uint32        GetFreeSize() const { return CHUNK_SIZE - _usedSize; }

private:
    void  Close(uint32 writeSize);
    BYTE* GetWritePos() { return _buffer + _usedSize; }

    static constexpr uint32 CHUNK_SIZE = Config::Buffer::SEND_BUFFER_CHUNK_SIZE;

    BYTE   _buffer[CHUNK_SIZE] = {};
    uint32 _usedSize = 0;
    bool   _open = false;
};


struct alignas(MEMORY_ALLOCATION_ALIGNMENT) SendBufferChunkNode : public SLIST_ENTRY
{
    SendBufferChunk chunk;
};

class SendBufferManager
{
public:
    SendBufferManager();
    ~SendBufferManager();

    SendBufferRef Open(uint32 allocSize);
    void          Push(SendBufferChunk* chunk);

private:
    SendBufferChunkRef Pop();
    static void        PushGlobal(SendBufferChunk* chunk);

    SLIST_HEADER _listHeader;
};