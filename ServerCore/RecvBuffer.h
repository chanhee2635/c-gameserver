#pragma once

class RecvBuffer
{
public:
    explicit RecvBuffer(uint32 bufferSize = Config::Session::RECV_BUFFER_SIZE);

    uint32                 GetWriteSegments(WSABUF wsaBufs[2]);
    std::span<const BYTE>  ReadSegment() const;
    void                   Linearize();

    uint32 GetFreeSize() const { return _capacity - _dataSize; }
    uint32 GetDataSize() const { return _dataSize; }
    bool   OnWrite(uint32 numOfBytes);
    bool   OnRead(uint32 numOfBytes);

private:
    uint32       _bufferSize = 0;
    uint32       _capacity   = 0;
    uint32       _readPos    = 0;
    uint32       _writePos   = 0;
    uint32       _dataSize   = 0;
    Vector<BYTE> _buffer;
};
