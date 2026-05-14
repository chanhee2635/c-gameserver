#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(uint32 bufferSize)
    : _bufferSize(bufferSize)
    , _capacity(bufferSize)
    , _buffer(_capacity)
{}

uint32 RecvBuffer::GetWriteSegments(WSABUF wsaBufs[2])
{
    uint32 freeSize = GetFreeSize();
    if (freeSize == 0)
        return 0;

    uint32 tailSize = _capacity - _writePos;

    if (tailSize >= freeSize)
    {
        wsaBufs[0].buf = reinterpret_cast<char*>(_buffer.data() + _writePos);
        wsaBufs[0].len = static_cast<ULONG>(freeSize);
        return 1;
    }

    wsaBufs[0].buf = reinterpret_cast<char*>(_buffer.data() + _writePos);
    wsaBufs[0].len = static_cast<ULONG>(tailSize);
    wsaBufs[1].buf = reinterpret_cast<char*>(_buffer.data());
    wsaBufs[1].len = static_cast<ULONG>(freeSize - tailSize);
    return 2;
}

std::span<const BYTE> RecvBuffer::ReadSegment() const
{
    uint32 tailSize = _capacity - _readPos;
    uint32 contiguous = std::min(_dataSize, tailSize);
    return { _buffer.data() + _readPos, contiguous };
}

void RecvBuffer::Linearize()
{
    if (_readPos + _dataSize <= _capacity)
        return;

    Vector<BYTE> temp(_dataSize);
    uint32 tailSize = _capacity - _readPos;

    ::memcpy(temp.data(), _buffer.data() + _readPos, tailSize);
    ::memcpy(temp.data() + tailSize, _buffer.data(), _dataSize - tailSize);
    ::memcpy(_buffer.data(), temp.data(), _dataSize);

    _readPos = 0;
    _writePos = _dataSize;
}

bool RecvBuffer::OnWrite(uint32 numOfBytes)
{
    if (numOfBytes > GetFreeSize())
        return false;

    _writePos = (_writePos + numOfBytes) % _capacity;
    _dataSize += numOfBytes;
    return true;
}

bool RecvBuffer::OnRead(uint32 numOfBytes)
{
    if (numOfBytes > GetDataSize())
        return false;

    _readPos = (_readPos + numOfBytes) % _capacity;
    _dataSize -= numOfBytes;

    if (_dataSize == 0)
        _readPos = _writePos = 0;

    return true;
}

