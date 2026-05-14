using System;

public sealed class RecvBuffer
{
    private readonly byte[] _buffer;
    private int _readPos;
    private int _writePos;

    public RecvBuffer(int bufferSize) => _buffer = new byte[bufferSize];

    public int DataSize => _writePos - _readPos;
    public int FreeSize => _buffer.Length - _writePos;

    public ArraySegment<byte> WriteSegment => new(_buffer, _writePos, FreeSize);
    public ArraySegment<byte> ReadSegment => new(_buffer, _readPos, DataSize);

    public bool OnRead(int numOfBytes)
    {
        if (numOfBytes > DataSize) return false;
        _readPos += numOfBytes;
        if (_readPos == _writePos) _readPos = _writePos = 0;
        return true;
    }

    public bool OnWrite(int numOfBytes)
    {
        if (numOfBytes > FreeSize)
        {
            if (DataSize + numOfBytes > _buffer.Length) return false;
            Buffer.BlockCopy(_buffer, _readPos, _buffer, 0, DataSize);
            _writePos = DataSize;
            _readPos = 0;
        }
        _writePos += numOfBytes;
        return true;
    }
}