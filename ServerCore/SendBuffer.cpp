#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
    : _owner(owner)
    , _buffer(buffer)
    , _allocSize(allocSize)
{}

void SendBuffer::Close(uint32 writeSize)
{
    ASSERT_CRASH(writeSize <= _allocSize);
    _writeSize = writeSize;
    _owner->Close(writeSize);
}

void SendBufferChunk::Reset()
{
    _usedSize = 0;
    _open = false;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize, SendBufferChunkRef self)
{
    ASSERT_CRASH(allocSize <= GetFreeSize());
    ASSERT_CRASH(!_open);

    _open = true;
    return MakeShared<SendBuffer>(self, GetWritePos(), allocSize);
}
void SendBufferChunk::Close(uint32 writeSize)
{
    ASSERT_CRASH(_open);
    _usedSize += writeSize;
    _open = false;
}

SendBufferManager::SendBufferManager()
{
    ::InitializeSListHead(&_listHeader);
}


SendBufferManager::~SendBufferManager()
{
    SLIST_ENTRY* entry;
    while ((entry = ::InterlockedPopEntrySList(&_listHeader)) != nullptr)
        xdelete(static_cast<SendBufferChunkNode*>(entry));
}

SendBufferRef SendBufferManager::Open(uint32 allocSize)
{
    ASSERT_CRASH(allocSize <= Config::Buffer::SEND_BUFFER_CHUNK_SIZE);

    if (LSendBufferChunk == nullptr)
        LSendBufferChunk = Pop();

    ASSERT_CRASH(!LSendBufferChunk->IsOpen());

    if (LSendBufferChunk->GetFreeSize() < allocSize)
        LSendBufferChunk = Pop();

    return LSendBufferChunk->Open(allocSize, LSendBufferChunk);
}

SendBufferChunkRef SendBufferManager::Pop()
{
    SLIST_ENTRY* entry = ::InterlockedPopEntrySList(&_listHeader);
    if (entry != nullptr)
    {
        SendBufferChunkNode* node = static_cast<SendBufferChunkNode*>(entry);
        return SendBufferChunkRef(&node->chunk, PushGlobal);
    }

    SendBufferChunkNode* node = xnew<SendBufferChunkNode>();
    return SendBufferChunkRef(&node->chunk, PushGlobal);
}

void SendBufferManager::Push(SendBufferChunk* chunk)
{
    SendBufferChunkNode* node = CONTAINING_RECORD(chunk, SendBufferChunkNode, chunk);
    ::InterlockedPushEntrySList(&_listHeader, node);
}

void SendBufferManager::PushGlobal(SendBufferChunk* chunk)
{
    if (GSendBufferManager == nullptr)
    {
        SendBufferChunkNode* node = CONTAINING_RECORD(chunk, SendBufferChunkNode, chunk);
        xdelete(node);
        return;
    }

    chunk->Reset();
    GSendBufferManager->Push(chunk);
}
