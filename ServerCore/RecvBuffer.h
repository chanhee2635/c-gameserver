#pragma once

/*--------------------
	  RecvBuffer
	  (순환 버퍼, memmove 제거)
---------------------*/

/*
* @brief TCP 수신 데이터를 저장하는 순환(Circular) 버퍼
* @details
*  - _readPos / _writePos 는 실제 배열 인덱스 [0, _capacity)
*  - _dataSize 로 채워진 바이트 수를 별도 추적 → 오버플로우 없이 정확한 상태 유지
*  - GetWriteSegments(): wrap-around 시 최대 2개 WSABUF 반환 → WSARecv scatter/gather
*  - Linearize()       : wrap 여부에 관계없이 연속 메모리 포인터 반환 (wrap 시에만 내부 복사)
*/
class RecvBuffer
{
public:
	explicit RecvBuffer(int32 capacity);
	~RecvBuffer() = default;

	// ─── 쓰기(WSARecv 완료) ──────────────────────────────
	/*
	* @brief WSARecv scatter/gather 용 쓰기 가능 세그먼트 반환
	* @param outSegs WSABUF[2] 배열 (최대 2개 채워질 수 있음)
	* @return 실제 채워진 세그먼트 개수 (0 = 버퍼 가득)
	*/
	int32		GetWriteSegments(WSABUF outSegs[2]);

	/*
	* @brief WSARecv 완료 후 쓰기 커서를 n바이트 전진
	*/
	bool		OnWrite(int32 numOfBytes);

	// ─── 읽기(OnRecv 처리) ───────────────────────────────
	/*
	* @brief 읽기 가능한 데이터를 연속 메모리로 반환
	* @details wrap 없으면 내부 포인터 그대로, wrap 있으면 임시 버퍼에 복사 후 반환
	* @return nullptr if DataSize() == 0
	*/
	BYTE*		Linearize();

	/*
	* @brief OnRecv 처리 후 읽기 커서를 n바이트 전진
	*/
	bool		OnRead(int32 numOfBytes);

	// ─── 상태 ────────────────────────────────────────────
	int32		DataSize() const { return _dataSize; }
	int32		FreeSize() const { return _capacity - _dataSize; }
	bool		IsEmpty()  const { return _dataSize == 0; }

private:
	int32			_capacity = 0;
	int32			_readPos  = 0;   // 실제 배열 인덱스 [0, _capacity)
	int32			_writePos = 0;   // 실제 배열 인덱스 [0, _capacity)
	int32			_dataSize = 0;   // 현재 보유 바이트 수

	Vector<BYTE>	_buffer;         // 순환 데이터 버퍼
	Vector<BYTE>	_linearBuf;      // Linearize() 임시 버퍼 (wrap 발생 시만 사용)
};
