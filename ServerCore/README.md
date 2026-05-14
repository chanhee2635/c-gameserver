# ServerCore

IOCP 기반 C++ 게임 서버 엔진 라이브러리입니다.  
`Server` 프로젝트가 이 라이브러리를 참조하여 게임 서버를 구성합니다.

---

## 목차

1. [기반 타입 및 설정](#1-기반-타입-및-설정)
2. [메모리 관리](#2-메모리-관리)
3. [네트워크 (IOCP)](#3-네트워크-iocp)
4. [버퍼](#4-버퍼)
5. [잡(Job) 시스템](#5-잡job-시스템)
6. [스레드](#6-스레드)
7. [DB (ODBC)](#7-db-odbc)
8. [로깅 / 통계](#8-로깅--통계)
9. [유틸리티](#9-유틸리티)
10. [전역 객체 (CoreGlobal)](#10-전역-객체-coreglobal)

---

## 1. 기반 타입 및 설정

### `Types.h`
전체 프로젝트에서 사용하는 타입 별칭을 정의합니다.

- **정수 타입**: `int8` ~ `int64`, `uint8` ~ `uint64`, `intptr`, `uintptr`
- **STL 별칭**: `string`, `wstring`, `Mutex`, `Thread`, `CondVar`, `UniqueLock`, `LockGuard`, `Atomic<T>`
- **스마트 포인터 별칭**: `SessionRef`, `ServiceRef`, `SendBufferRef`, `JobQueueRef` 등 주요 클래스의 `shared_ptr` / `weak_ptr`
- **유틸 매크로**: `size16(val)`, `size32(val)` — sizeof를 int16/int32로 캐스팅, `len16` / `len32` — 배열 길이 반환
- **열거형**:
  - `AllocType` — `Frame`, `Stomp`, `Pool` (메모리 할당 전략)
  - `ThreadType` — `NONE`, `WORKER`, `DB`, `MONITOR` (스레드 역할 구분)

---

### `CoreConfig.h`
전체 엔진의 상수값을 `Config` 네임스페이스로 관리합니다.

| 네임스페이스 | 상수 | 값 | 설명 |
|-------------|------|----|------|
| `Memory` | `MAX_POOL_SIZE` | 4096 | 풀로 관리하는 최대 블록 크기 (byte) |
| `Memory` | `POOL_COUNT` | 40 | 풀 종류 수 (32B 단위 32개 + 128B 단위 8개 등) |
| `Memory` | `TLS_MAX_COUNT` | 32 | TLS 풀 최대 보유 블록 수 |
| `Memory` | `TLS_BATCH_COUNT` | 16 | TLS ↔ 글로벌 풀 배치 이동 단위 |
| `Buffer` | `SEND_BUFFER_CHUNK_SIZE` | 4KB | 송신 버퍼 청크 크기 |
| `Session` | `RECV_BUFFER_SIZE` | 4KB | 수신 버퍼 단위 크기 |
| `Session` | `RECV_BUFFER_COUNT` | 10 | Circular 모드 버퍼 배수 (Linear 전용) |
| `Session` | `MAX_PACKET_SIZE` | 4KB | 허용하는 최대 패킷 크기 |
| `Job` | `MAX_WORK_TICK` | 64ms | 워커 스레드가 한 JobQueue를 연속 처리하는 최대 시간 |

---

### `CoreMacro.h`
자주 사용하는 매크로를 정의합니다.

```cpp
OUT                         // 출력 파라미터 표시 (의미 없는 빈 매크로)

USE_LOCK                    // 클래스 내 shared_mutex _rwLock 선언
READ_LOCK                   // shared_lock 획득 (다중 읽기 허용)
WRITE_LOCK                  // unique_lock 획득 (단독 쓰기)

CRASH(cause)                // __debugbreak() + 메시지 출력
ASSERT_CRASH(expr)          // expr가 false일 때 CRASH 호출
```

---

### `Container.h`
STL 컨테이너를 `StlAllocator` 기반으로 별칭 처리합니다.

- `Vector<T>`, `List<T>`, `Queue<T>`, `Stack<T>`
- `HashMap<K,V>`, `HashSet<K>`, `Map<K,V>`, `Set<K>`
- `PriorityQueue<T, ...>`, `Deque<T>`
- `FrameVector<T>` — `FrameAllocator` 기반 (프레임 종료 시 자동 해제)

---

### `CoreTLS.h` / `CoreTLS.cpp`
각 스레드가 독립적으로 보유하는 Thread Local Storage 변수를 관리합니다.

| TLS 변수 | 타입 | 설명 |
|---------|------|------|
| `LThreadType` | `ThreadType` | 현재 스레드의 역할 (WORKER / DB / MONITOR) |
| `LThreadId` | `uint32` | 스레드 고유 ID |
| `LEndTickCount` | `uint64` | 워커 루프 종료 기준 tick |
| `LSendBufferChunk` | `SendBufferChunkRef` | 스레드 로컬 송신 버퍼 청크 |
| `LCurrentJobQueue` | `JobQueue*` | 현재 실행 중인 JobQueue 포인터 |
| `LThreadMemory` | `ThreadLocalMemory*` | 스레드 로컬 메모리 풀 |
| `LFrameAllocator` | `FrameAllocator*` | 프레임 단위 임시 할당기 |

`CoreTLS::OnThreadStart(type, id)` — 스레드 시작 시 TLS 초기화  
`CoreTLS::OnThreadEnd()` — 스레드 종료 시 TLS 해제 (메모리 반납 포함)

---

## 2. 메모리 관리

### `Allocator.h` / `Allocator.cpp`

4가지 할당 전략을 제공합니다.

#### `BaseAllocator`
- `_aligned_malloc` / `_aligned_free` 기반 힙 할당
- 16바이트 정렬 보장
- 풀 크기 초과 or 글로벌 fallback 용도

#### `FrameAllocator`
- 스레드당 1MB의 선형 버퍼를 `_aligned_malloc`으로 미리 할당
- `Allocate()` — 포인터를 앞으로 밀기만 하므로 O(1) 할당
- `Release()` — 아무 작업 없음 (해제는 `Clear()`로 일괄 처리)
- `Clear()` — `_freePtr`을 버퍼 시작으로 리셋
- 프레임 단위로 생존하는 임시 데이터용 (`FrameVector` 등)

#### `PoolAllocator`
- 내부적으로 `GMemory->Allocate()` / `GMemory->Release()` 호출
- 크기별 풀에서 블록을 재사용

#### `StompAllocator`
- `VirtualAlloc`으로 페이지 단위 메모리 할당
- 데이터 영역 직후에 접근 불가(`PAGE_NOACCESS`) 가드 페이지 설치
- 버퍼 오버런 발생 시 즉시 접근 위반 → 디버깅 목적

#### `StlAllocator<T, AllocType>`
- STL 컨테이너에 주입하는 커스텀 Allocator
- `AllocType::Pool` (기본) / `Frame` / `Stomp` 선택 가능

---

### `MemoryPool.h` / `MemoryPool.cpp`

#### `MemoryHeader`
- `SLIST_ENTRY` 상속 → Windows `SLIST` (lock-free 스택) 연결
- 모든 할당 블록 앞에 붙는 헤더 (allocSize + magic number 보관)
- `AttachHeader` / `DetachHeader` — 헤더 포인터 ↔ 사용자 포인터 변환

#### `MemoryPool` (글로벌 풀)
- `SLIST_HEADER` 기반 lock-free 스택
- `Pop()` — 스택에서 블록 꺼냄, 없으면 `AllocBatch()`로 32개 일괄 할당
- `Push()` — magic number 검증 후 반납 (이중 해제 감지)
- Debug 모드에서 `_useCount` / `_reserveCount` 추적

#### `TlsMemoryPool` (스레드 로컬 풀)
- 글로벌 풀을 스레드별로 캐싱 (lock 경합 최소화)
- `Pop()` — 로컬 캐시가 비면 글로벌 풀에서 `BatchCount(16)` 개 가져옴
- `Push()` — 로컬 캐시가 `MaxCount(32)` 초과 시 글로벌로 반납
- 스레드 종료 시 `ReturnAll()` — 보유 블록 전량 글로벌 반납

---

### `Memory.h`

#### `MemoryManager`
- 크기별 `MemoryPool` 배열과 `_poolTable[0..4096]` 룩업 테이블 관리
- `Allocate(size)` — 4096 이하면 풀에서, 초과면 `BaseAllocator`에서 할당
- `Release(ptr)` — `MemoryHeader`에서 allocSize를 읽어 적절한 풀로 반납

#### 전역 헬퍼 함수
```cpp
xnew<T>(args...)      // PoolAllocator 할당 후 placement new
xdelete<T>(ptr)       // 소멸자 호출 후 PoolAllocator 반납
MakeShared<T>(args...) // xnew + shared_ptr (커스텀 deleter = xdelete)
```

---

## 3. 네트워크 (IOCP)

### `IocpCore.h` / `IocpCore.cpp`

#### `IocpObject`
- IOCP에 등록 가능한 객체의 기반 클래스 (`enable_shared_from_this` 상속)
- `GetHandle()` — IOCP에 등록할 핸들 반환
- `Dispatch(event, bytes)` — IOCP 완료 이벤트 처리

#### `IocpCore`
- `CreateIoCompletionPort`로 IOCP 핸들 생성
- `Register(obj)` — 소켓/핸들을 IOCP에 연결
- `Dispatch(timeoutMs)` — `GetQueuedCompletionStatus` 호출 후 `ProcessEvent`로 위임
- `ProcessEvent` — Debug 빌드에서 처리 시간(µs) 측정 후 `GServerStats->iocp` 업데이트

---

### `IocpEvent.h` / `IocpEvent.cpp`
OVERLAPPED 구조체를 상속한 IOCP 이벤트 래퍼입니다.

- 이벤트 타입: `Recv`, `Send`, `Connect`, `Accept`
- `Init()` — OVERLAPPED 영역 초기화 (재사용 전 필수)
- `SetOwner(ref)` / `GetOwner()` — 이벤트 소유 객체의 shared_ptr (수명 관리)

---

### `NetAddress.h` / `NetAddress.cpp`
IP 주소와 포트를 `SOCKADDR_IN`으로 래핑합니다.

- `NetAddress(ip, port)` — `InetPtonW`로 문자열 IP → 바이너리 변환
- `GetIpAddress()` — `InetNtopW`로 바이너리 → 문자열 IP 변환
- `GetSockAddr()` — `SOCKADDR_IN` 반환

---

### `SocketUtils.h` / `SocketUtils.cpp`
WinSock 초기화 및 소켓 유틸리티를 제공합니다.

- `Init()` / `Clear()` — WSAStartup / WSACleanup
- `CreateSocket()` — OVERLAPPED 속성의 TCP 소켓 생성
- `SetLinger`, `SetReuseAddress`, `SetRecvBufferSize`, `SetSendBufferSize`, `SetTcpNoDelay` — 소켓 옵션
- `BindAnyAddress(socket, port)` — 임의 주소 바인딩 (ConnectEx 전 필수)
- `ConnectEx` / `AcceptEx` 함수 포인터 로드 및 호출 래퍼

---

### `Listener.h` / `Listener.cpp`
`AcceptEx` 기반 비동기 다중 Accept 처리입니다.

- `StartAccept(service)` — `DEFAULT_ACCEPT_COUNT(100)` 개의 AcceptEvent를 미리 등록
- IOCP 완료 → `Dispatch()` → `ProcessAccept()` → 새 세션 생성 및 `ProcessConnect()` 호출 → 다시 `RegisterAccept()`

---

### `Session.h` / `Session.cpp`

#### `Session`
IOCP 기반 TCP 연결 단위 클래스입니다.

**수신 흐름**
1. `RegisterRecv()` — `WSARecv`에 RecvBuffer의 쓰기 가능 영역(`GetWriteSegments`) 등록 (Circular 모드에서 최대 2 세그먼트)
2. IOCP 완료 → `ProcessRecv()` → `RecvBuffer::OnWrite()` → `OnRecv()` 반복 호출
3. 데이터가 랩어라운드되면 `Linearize()` 후 재시도

**송신 흐름**
1. `Send(buffer)` — `_sendQueue`에 버퍼 추가, `_isSendRegistered`가 false면 `RegisterSend()` 호출
2. `RegisterSend()` — `_sendQueue`를 `_sendPendingList`로 swap, `WSASend`에 WSABUF 배열 등록 (gather write)
3. IOCP 완료 → `ProcessSend()` → 남은 큐가 있으면 즉시 다음 `RegisterSend()`

**생명주기**
- `ProcessConnect()` — AcceptEx / ConnectEx 완료 시 호출, `OnConnected()` + `RegisterRecv()`
- `Disconnect()` — atomic exchange로 중복 호출 방지, `OnDisconnected()` + 서비스에서 세션 제거

#### `PacketSession` (Session 상속)
- `OnRecv()` final — `PacketHeader(size 2B + type 2B)` 기준으로 패킷 분리
- 패킷 크기 검증 (`sizeof(Header)` ~ `MAX_PACKET_SIZE`)
- 완성된 패킷마다 `OnRecvPacket(span, type)` 호출 및 `recvPackets` 통계 업데이트

---

### `Service.h` / `Service.cpp`

#### `Service` (기반)
- 세션 팩토리(`SessionFactory`), IOCP 코어, 세션 목록 관리
- `CreateSession()` — 팩토리로 세션 생성 후 IOCP 등록
- `AddSession` / `ReleaseSession` — 세션 목록과 카운터 동기화
- `ForEachSession(fn)` — 락 보호 하에 모든 세션 순회

#### `ServerService`
- `Start()` — `Listener` 생성 후 `StartAccept()` 호출

#### `ClientService`
- `Start()` — `maxSessionCount`만큼 `Connect()` 반복 호출 (아웃바운드 연결)

---

## 4. 버퍼

### `RecvBuffer.h` / `RecvBuffer.cpp`

**두 가지 모드를 지원합니다:**

#### Circular 모드 (기본)
- 용량: `RECV_BUFFER_SIZE(4KB)`
- `GetWriteSegments()` — 빈 공간이 랩어라운드되면 WSABUF 2개 반환 (scatter-gather)
- `OnRead()` — readPos를 앞으로, 버퍼가 비면 readPos/writePos 모두 0으로 리셋 (캐시 효율)
- `Linearize()` — 데이터가 버퍼 끝을 넘어 랩어라운드된 경우 연속 메모리로 재배치

#### Linear 모드
- 용량: `RECV_BUFFER_SIZE × RECV_BUFFER_COUNT(10)` = 40KB
- 단순 포인터 전진 방식, 남은 공간 부족 시 `CleanLinear()`로 memmove

---

### `SendBuffer.h` / `SendBuffer.cpp`

#### `SendBufferChunk`
- 4KB 고정 청크, 한 번에 하나의 버퍼만 Open 가능
- `Open(size)` → `SendBuffer` 반환 → 데이터 기록 → `Close(writeSize)` → usedSize 전진

#### `SendBufferManager`
- TLS(`LSendBufferChunk`)에 청크를 캐싱하여 청크 할당 lock 경합 최소화
- 청크 소진 시 `Pop()` — 전역 풀에서 꺼냄, 없으면 `xnew<SendBufferChunk>()` 생성
- 청크 반납 시 `PushGlobal()` (커스텀 deleter) — `Reset()` 후 전역 풀 반환

---

### `BufferReader.h` / `BufferWriter.h`
패킷 직렬화/역직렬화용 헬퍼 클래스입니다.

- `BufferReader` — `operator>>` 로 버퍼에서 값 읽기, 경계 검사 포함
- `BufferWriter` — `operator<<` 로 버퍼에 값 쓰기

---

## 5. 잡(Job) 시스템

### `JobQueue.h` / `JobQueue.cpp`

모든 비동기 작업의 기반 클래스입니다. `enable_shared_from_this` 상속 필수입니다.

#### 동작 원리
1. `DoAsync(job)` → `Push(job)` → 큐에 적재
2. 큐가 idle(`_pending == false`) 상태였다면 `GGlobalQueue`에 자신을 등록
3. Worker 스레드가 `GGlobalQueue`에서 꺼내 `Execute()` 호출
4. `Execute()` — 최대 `MAX_WORK_TICK(64ms)` 동안 Job 처리, 시간 초과 시 다시 GGlobalQueue에 등록

#### 멤버 함수 오버로드
```cpp
DoAsync(job)                          // 람다 / std::function
DoAsync(&MyClass::Method, args...)    // 멤버 함수 포인터 (자동으로 shared_from_this 캡처)
DoTimer(afterMs, job)                 // GJobTimer에 지연 예약
DoTimer(afterMs, &MyClass::Method, args...)
```

---

### `JobTimer.h` / `JobTimer.cpp`

지연 실행 예약을 관리하는 우선순위 큐입니다.

- `Reserve(afterMs, owner, job)` — `GetTickCount64() + afterMs`를 실행 tick으로 `TimerItem` 삽입
- `Distribute(nowTick)` — Worker 루프에서 매 프레임 호출, 만료된 Item을 해당 `JobQueue::Push()`로 위임
- `_distributing` atomic flag — 동시에 여러 스레드가 Distribute 진입하는 것을 방지

---

### `GlobalQueue.h` / `GlobalQueue.cpp`

Worker 스레드 간 JobQueue를 분배하는 공유 큐입니다.

- 내부 구조: `LockQueue<JobQueueRef>` (mutex 기반 스레드 안전)
- `Push(jobQueue)` — 처리할 JobQueue 추가
- `TryPop(OUT jobQueue)` — 즉시 반환, 없으면 false

---

### `LockQueue.h`

`std::mutex` 기반 범용 스레드 안전 큐입니다.

- `Push(item)` — lock 후 enqueue
- `TryPop(OUT item)` — 비어있으면 false 반환

---

## 6. 스레드

### `ThreadManager.h` / `ThreadManager.cpp`

스레드 생성과 생명주기를 관리합니다.

- `InitMainThread(type)` — 메인 스레드에 TLS 초기화 (TID 발급)
- `Launch(type, callback, args...)` — `std::thread` 생성, 내부에서 `CoreTLS::OnThreadStart` / `OnThreadEnd` 자동 호출
- `Join()` — 모든 스레드 완료 대기 후 목록 초기화
- `DoGlobalQueueWork()` — Worker 루프 본체
  - `LEndTickCount` 이전까지 `GGlobalQueue->TryPop()` + `Execute()` 반복

---

## 7. DB (ODBC)

### `DbConnection.h` / `DbConnection.cpp`

단일 ODBC 커넥션 래퍼입니다.

- `Connect(env, connectionString)` — `SQLDriverConnectW`로 연결, Statement 핸들 할당
- `Clear()` — Statement / Connection 핸들 해제
- `Prepare(query)` — `SQLPrepareW`로 쿼리 준비 (바인딩 후 실행)
- `Execute()` — `SQLExecute` (Prepare 후 사용)
- `Execute(query)` — `SQLExecDirectW` (즉시 실행, 파라미터 없는 쿼리)
- `Fetch()` — `SQLFetch`로 결과 행 이동
- `GetRowCount()` — `SQLRowCount`로 영향받은 행 수
- `Unbind()` — `SQL_RESET_PARAMS` + `SQL_UNBIND` + `SQL_CLOSE` (재사용 전 필수)

**타입별 오버로드 (BindParam / BindCol)**

| C++ 타입 | SQL 타입 | C 타입 |
|---------|---------|--------|
| `bool` | `SQL_TINYINT` | `SQL_C_TINYINT` |
| `int8` / `int16` / `int32` / `int64` | `SQL_TINYINT` ~ `SQL_BIGINT` | 대응 C 타입 |
| `uint64` | `SQL_BIGINT` | `SQL_C_UBIGINT` |
| `float` | `SQL_REAL` | `SQL_C_FLOAT` |
| `double` | `SQL_DOUBLE` | `SQL_C_DOUBLE` |
| `WCHAR*` | `SQL_WVARCHAR` | `SQL_C_WCHAR` |
| `BYTE*` (binary) | `SQL_BINARY` | `SQL_C_BINARY` |

---

### `DbConnectionPool.h` / `DbConnectionPool.cpp`

커넥션 풀을 관리합니다. `GDbConnectionPool` 전역 포인터로 접근합니다.

- `Init(connectionString, count)` — ODBC 환경 핸들 초기화 후 `count`개 커넥션 생성 및 Connect
- `Clear()` — 모든 커넥션 `Clear()` 후 환경 핸들 해제
- `Pop()` — 사용 가능한 커넥션 반환 (없으면 nullptr)
- `Push(conn)` — 사용 완료된 커넥션 반납
- 내부에서 `USE_LOCK` / `WRITE_LOCK` 사용 (스레드 안전)

---

### `DbBind.h`

컴파일 타임에 파라미터/컬럼 개수를 검증하는 쿼리 헬퍼 템플릿입니다.

```cpp
DbBind<ParamCount, ColumnCount> bind(conn, query);
```

- `BindParam(value)` — 순서대로 파라미터 바인딩 (인덱스 자동 증가)
- `BindCol(value)` — 순서대로 컬럼 바인딩
- `Execute()` — `ASSERT_CRASH`로 실제 바인딩 수 검증 후 `Prepare` + `Execute` 호출

**RAII 헬퍼 패턴 (DBManager 내부)**
```cpp
struct DbConnGuard {
    DbConnection* conn = nullptr;
    DbConnGuard()  { conn = GDbConnectionPool->Pop(); }
    ~DbConnGuard() { if (conn) { conn->Unbind(); GDbConnectionPool->Push(conn); } }
    bool Valid() const { return conn != nullptr; }
};
```

**쿼리 작성 예시**
```cpp
DbConnGuard g;
if (!g.Valid()) return false;

uint64 accountId = 1001;
uint64 outPlayerId = 0;
WCHAR  outName[100] = {};
int32  outLevel = 0;

DbBind<1, 3> bind(*g.conn,
    L"SELECT player_id, name, level FROM Players WHERE account_id = ?");
bind.BindParam(accountId);
bind.BindCol(outPlayerId);
bind.BindCol(outName, 100);
bind.BindCol(outLevel);

if (!bind.Execute()) return false;
while (g.conn->Fetch()) { /* 결과 처리 */ }
```

---

## 8. 로깅 / 통계

### `Logger.h` / `Logger.cpp`

비동기 로거입니다. 호출 스레드를 블로킹하지 않습니다.

- `Init(filename)` — 로그 파일 열기 + MONITOR 타입 스레드(`LogThreadMain`) 시작
- `Write(level, msg)` — `LogEntry{level, msg, threadId}`를 큐에 push 후 `cv.notify_one()`
- `Shutdown()` — `_running = false` 후 `cv.notify_all()` (로그 스레드 종료)
- `LogThreadMain()` — 큐에서 꺼내 파일 + 콘솔에 출력, 형식: `HH:MM:SS [LEVEL] [TID] message`
- `ReserveStatsPanel(lines)` — 통계 패널 영역을 콘솔 상단에 예약 (ANSI 이스케이프 활성화)
- 로그 출력 시 커서가 통계 영역 안에 있으면 경계 아래로 강제 이동

**매크로**
```cpp
LOG_INFO(msg)   // LogLevel::INFO
LOG_WARN(msg)   // LogLevel::WARN
LOG_ERROR(msg)  // LogLevel::ERR
```

---

### `ServerStats.h` / `ServerStats.cpp`

서버 운영 지표를 실시간으로 수집하고 콘솔에 렌더링합니다.

| 구조체 | 수집 항목 |
|--------|---------|
| `NetworkStats` | recvBytes, sendBytes, recvPackets, sendPackets |
| `JobStats` | jobsExecuted(초당), timerFired |
| `IocpStats` | iocpCallCount(초당), totalProcessTimeUs (Debug) |
| `MemoryStats` | poolHitCount, poolMissCount, liveAllocCount |
| `GameMetrics` | connectedSessions, totalConnections, activePlayers, activeMonsters, uptime |
| `SystemStats` | cpuUsage(%), memUsageMB — `GetProcessTimes` / `GetProcessMemoryInfo` 활용 |

- `Init()` — `ReserveStatsPanel(10)`으로 콘솔 상단 10줄 확보
- `RunUpdateLoop()` — 1초마다 `RenderReport()` 호출 (MONITOR 스레드에서 실행)
- `RenderReport()` — ANSI 이스케이프로 콘솔 최상단(0,0)에 통계 패널 덮어쓰기, 로그 커서 위치 복원

---

### `ConsoleLog.h` / `ConsoleLog.cpp`
콘솔 색상 출력 유틸리티입니다. (INFO=흰색, WARN=노란색, ERROR=빨간색)

---

## 9. 유틸리티

### `Utils.h`
```cpp
Utils::s2ws(str)      // std::string → std::wstring
Utils::ws2s(wstr)     // std::wstring → std::string
Utils::ToString(wstr) // wstring → string (protobuf용)
```

### `TypeCast.h`
`ObjectPool` 기반 안전한 타입 다운캐스팅 헬퍼입니다.

### `ObjectPool.h`
특정 타입 `T`에 특화된 오브젝트 풀 (`xnew<T>` / `xdelete<T>` 내부 사용)

---

## 10. 전역 객체 (CoreGlobal)

### `CoreGlobal.h` / `CoreGlobal.cpp`

엔진 전역 객체의 생명주기를 관리합니다.

**소유 패턴**: `static unique_ptr<T>` (소유) + `T* GXxx` (접근용 전역 포인터)

### 초기화 순서 (`CoreGlobal::Init`)

| 순서 | 전역 포인터 | 이유 |
|------|------------|------|
| 1 | `GMemory` | 이후 모든 xnew/xdelete가 GMemory 참조 |
| 2 | `SocketUtils::Init()` | — |
| 3 | `GThread` | Logger::Init()이 GThread->Launch() 호출 |
| 4 | `GSendBufferManager` | — |
| 5 | `GGlobalQueue` | JobQueue::Push가 GGlobalQueue 참조 |
| 6 | `GJobTimer` | JobQueue::DoTimer가 GJobTimer 참조 |
| 7 | `GLogger` | 이후 LOG_* 매크로 사용 가능 |
| 8 | `GServerStats` | GLogger 준비 후 통계 스레드 시작 |
| 9 | `GDbConnectionPool` | 객체만 생성, 실제 연결은 GameGlobal에서 |

### 해제 순서 (`CoreGlobal::Clear`)
초기화의 역순으로 해제합니다. Logger 소멸자에서 로그 스레드를 종료하고, ThreadManager::Join()으로 모든 Worker 스레드 완료를 대기합니다.

---

## 의존 관계 요약

```
CoreConfig.h ←─ 모든 파일
Types.h      ←─ 모든 파일
CoreMacro.h  ←─ 대부분 파일

MemoryPool ←── Memory ←── Allocator
                          ↑
CoreTLS ──────────────────┘ (LFrameAllocator, LThreadMemory)

IocpCore ←── Session ←── PacketSession
             ↑
Service ─────┘
Listener ────┘

JobQueue ──→ GlobalQueue (Push)
         ──→ JobTimer    (DoTimer)
ThreadManager ──→ GlobalQueue (DoGlobalQueueWork)

DbConnection ←── DbConnectionPool (GDbConnectionPool)
DbBind       ──→ DbConnection
```
