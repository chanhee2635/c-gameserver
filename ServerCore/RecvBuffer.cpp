#include "pch.h"
#include "RecvBuffer.h"
#include "ServerStats.h"

RecvBuffer::RecvBuffer(int32 capacity)
	: _capacity(capacity), _readPos(0), _writePos(0), _dataSize(0)
{
	ASSERT_CRASH(capacity > 0);
	_buffer.resize(capacity);
}

/*
* GetWriteSegments
*
*  버퍼 상태에 따라 WSARecv scatter/gather 세그먼트를 구성한다.
*
*  Case 1 – writePos >= readPos (또는 readPos == 0)
*            [ ... readPos ... writePos ... end ] [ 0 ... readPos ]
*            Seg1: [writePos, end)  →  min(_capacity - writePos, freeSize) 바이트
*            Seg2: [0, readPos)     →  있을 때만
*
*  Case 2 – writePos < readPos
*            [ ... writePos ... readPos ... ]
*            Seg1: [writePos, readPos)  →  freeSize 바이트, 한 세그먼트만
*/
int32 RecvBuffer::GetWriteSegments(WSABUF outSegs[2])
{
	int32 freeSize = FreeSize();
	if (freeSize == 0) return 0;

	if (_writePos >= _readPos)
	{
		// Seg1: writePos → end-of-buffer
		int32 seg1 = _capacity - _writePos;
		outSegs[0].buf = reinterpret_cast<char*>(&_buffer[_writePos]);
		outSegs[0].len = static_cast<ULONG>(seg1);

		// Seg2: 0 → readPos (빈 공간이 있을 때만)
		int32 seg2 = _readPos;  // [0, readPos) = readPos 바이트
		if (seg2 > 0)
		{
			outSegs[1].buf = reinterpret_cast<char*>(&_buffer[0]);
			outSegs[1].len = static_cast<ULONG>(seg2);
			return 2;
		}
		return 1;
	}
	else
	{
		// writePos < readPos: 연속 공간 하나뿐
		outSegs[0].buf = reinterpret_cast<char*>(&_buffer[_writePos]);
		outSegs[0].len = static_cast<ULONG>(_readPos - _writePos);
		return 1;
	}
}

bool RecvBuffer::OnWrite(int32 numOfBytes)
{
	if (numOfBytes <= 0 || numOfBytes > FreeSize()) return false;

	_writePos = (_writePos + numOfBytes) % _capacity;
	_dataSize += numOfBytes;
	return true;
}

/*
* Linearize
*
*  데이터가 wrap되지 않은 경우: 내부 버퍼 포인터를 그대로 반환 (O(1), 복사 없음)
*  데이터가 wrap된 경우       : _linearBuf 로 두 세그먼트를 이어붙인 뒤 포인터 반환 (O(n) 복사)
*
*  반환 포인터는 다음 OnWrite/OnRead 호출 전까지만 유효하다.
*/
BYTE* RecvBuffer::Linearize()
{
	if (_dataSize == 0) return nullptr;

	// wrap 없음: readPos → writePos 가 연속
	if (_writePos > _readPos || _writePos == 0)
	{
		return &_buffer[_readPos];
	}

	// wrap 발생: [readPos, end) + [0, writePos)
	ServerStats::Get().recvBuffer.linearizeCount.fetch_add(1, std::memory_order_relaxed);

	_linearBuf.resize(_dataSize);
	int32 firstPart = _capacity - _readPos;  // readPos → end
	::memcpy(_linearBuf.data(), &_buffer[_readPos], firstPart);
	::memcpy(_linearBuf.data() + firstPart, &_buffer[0], _dataSize - firstPart);
	return _linearBuf.data();
}

bool RecvBuffer::OnRead(int32 numOfBytes)
{
	if (numOfBytes <= 0 || numOfBytes > _dataSize) return false;

	_readPos = (_readPos + numOfBytes) % _capacity;
	_dataSize -= numOfBytes;
	return true;
}
