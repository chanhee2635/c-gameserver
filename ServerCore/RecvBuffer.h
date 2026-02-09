#pragma once

/*---------------
	RecvBuffer
----------------*/

/*
* @brief TCP 수신 데이터를 저장하고 관리하는 가변 선형 버퍼
* @details 커널이 데이터를 넣는 지점(WritePos)과 콘텐츠가 데이터를 읽는 지점(ReadPos)을 분리 관리
* 데이터가 쌓여 버퍼 끝에 도달하면 Clean()을 통해 데이터를 앞으로 당겨 공간 확보
*/
class RecvBuffer
{
public:
	RecvBuffer(int32 bufferSize, int32 bufferCount);
	~RecvBuffer();

	/*
	* @brief 버퍼의 빈 공간 확보를 위해 남은 데이터를 맨 앞으로 복사
	* @details 읽기/쓰기 커서가 동일한 위치라면 리셋하고, 남은 공간이 부족하면 메모리 복사를 수행한다.
	*/
	void				Clean();

	/*
	* @brief 데이터를 읽은 후 읽기 커서를 이동
	*/
	bool				OnRead(int32 numOfBytes);

	/*
	* @brief 커널이 데이터를 기록한 후 쓰기 커서를 이동
	*/
	bool				OnWrite(int32 numOfBytes);

	// 현재 읽어야할 데이터의 시작 주소
	BYTE*				ReadPos() { return &_buffer[_readPos]; }

	// 커널이 다음 데이터를 기록할 주소
	BYTE*				WritePos() { return &_buffer[_writePos]; }

	// 버퍼에 쌓여 아직 처리되지 않은 데이터 크기
	int32				DataSize() { return _writePos - _readPos; }

	// 커널이 추가로 데이터를 기록할 수 있는 남은 공간 크기
	int32				FreeSize() { return _capacity - _writePos; }

private:
	int32				_capacity = 0;
	int32				_bufferSize = 0;
	int32				_readPos = 0;
	int32				_writePos = 0;
	Vector<BYTE>		_buffer;
};

