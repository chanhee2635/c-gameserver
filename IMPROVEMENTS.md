# IMPROVEMENTS.md — 개선 포인트 상세 분석

이 문서는 현재 코드베이스의 잠재적 개선 영역을 우선순위별로 정리합니다.  
각 항목에는 현재 문제, 개선 방향, 예상 효과를 포함합니다.

---

## 우선순위 범례

| 기호 | 의미 |
|------|------|
| [HIGH] | 버그·장애 가능성 또는 성능에 직접 영향 |
| [MED] | 운영 품질·유지보수성 개선 |
| [LOW] | 장기적 확장성·코드 품질 향상 |

---

## 1. 네트워크 / Session 계층

### [HIGH] RecvBuffer 모드 선택이 런타임이므로 분기 비용 발생

**현재 상태**  
`RecvBuffer`는 생성 시 `BufferMode::Linear` 또는 `Circular`를 선택합니다. 내부 메서드에서 매번 `_mode` 분기가 발생합니다.

```cpp
// Session.h
RecvBuffer _recvBuffer{ BufferMode::Circular };
```

**개선 방향**  
템플릿 파라미터로 모드를 컴파일 타임에 결정하거나, `Linear` 코드 경로를 제거하고 `Circular` 전용으로 단순화합니다. 현재 GameServer는 항상 Circular를 사용하므로 사문화된 분기입니다.

**예상 효과**  
코드 복잡도 감소, 핫 패스 분기 제거.

---

### [HIGH] SendBufferManager::_lock 이 std::mutex (Session 송신 경합)

**현재 상태**  
`SendBufferManager`의 글로벌 청크 풀이 `std::mutex`로 보호됩니다.  
TLS 청크가 소진될 때마다 이 락을 획득하므로 Worker 스레드 수가 늘면 경합이 증가합니다.

```cpp
// SendBuffer.h
class SendBufferManager {
    std::mutex _lock;
    Vector<SendBufferChunkRef> _chunks;
};
```

**개선 방향**  
`SLIST_HEADER`(Windows Lock-free 스택) 또는 `std::atomic`을 이용한 CAS 스택으로 교체합니다. `MemoryPool`이 이미 `SLIST_HEADER`를 쓰고 있어 동일 패턴 적용이 자연스럽습니다.

**예상 효과**  
고부하 시 Worker 스레드간 락 경합 제거.

---

### [MED] Session::_sendQueue 에서 _sendLock 보유 시간이 길 수 있음

**현재 상태**  
`Send()` 호출 시 `_sendLock`을 잡고 `_sendQueue`에 push 후, `isSendRegistered`가 false이면 `RegisterSend()`까지 락 내에서 실행합니다. `RegisterSend()`는 `WSASend`를 호출하며 커널 진입이 발생할 수 있습니다.

**개선 방향**  
락 범위를 큐 push까지만으로 좁히고, `isSendRegistered` 플래그는 `std::atomic<bool>` CAS로 처리하여 락 없이 WSASend 등록 여부를 결정합니다.

**예상 효과**  
전송 경합 감소, 락 보유 시간 단축.

---

### [MED] Disconnect 이후 이미 큐에 있는 Send 처리 보장 없음

**현재 상태**  
`Session::Disconnect()`는 소켓을 닫지만 `_sendQueue`에 남아 있는 패킷들이 전송될 보장이 없습니다. 레이스 컨디션으로 `OnDisconnected` 이전에 종료 응답 패킷이 유실될 수 있습니다.

**개선 방향**  
Graceful Shutdown 패턴을 적용합니다. `Disconnect()` 호출 시 즉시 소켓을 닫지 않고 `SD_SEND`(`shutdown(sock, SD_SEND)`) 후 수신 완료를 대기합니다. 또는 `_pendingClose` 플래그를 두어 `ProcessSend` 완료 시점에 소켓을 닫도록 합니다.

**예상 효과**  
종료 응답 패킷 전송 보장, 클라이언트 오류 메시지 정상 수신.

---

## 2. 메모리 시스템

### [HIGH] MAX_POOL_SIZE(4096) 초과 객체는 매번 _aligned_malloc

**현재 상태**  
`PoolAllocator::Allocate`에서 4096 바이트를 초과하는 할당은 `BaseAllocator`(정렬 malloc)로 직접 처리됩니다. Protobuf 직렬화 버퍼나 대형 패킷 구성에서 빈번히 발생할 수 있습니다.

**개선 방향**  
`MAX_POOL_SIZE`를 `CoreConfig.h`에서 상향 조정하거나(8192 또는 16384), 대형 객체 전용 별도 풀 구간을 추가합니다.

**예상 효과**  
대형 할당의 OS 콜 빈도 감소.

---

### [MED] FrameAllocator의 1MB 상한 초과 시 assert/undefined behavior

**현재 상태**  
`FrameAllocator::Allocate`는 `_freePtr + size > _endPtr`이면 `nullptr`을 반환하거나 assert합니다. 워커 스레드가 1MB를 초과하는 프레임 할당을 수행하면 조용히 실패할 수 있습니다.

**개선 방향**  
오버플로 시 `BaseAllocator`로 폴백하고 로그를 남기거나, 버퍼 크기를 동적으로 확장합니다. 또는 `FrameAllocator` 사용 여부를 명확히 하고 초과 할당 경고를 ServerStats에 집계합니다.

**예상 효과**  
프레임 할당 실패 시 서버 안정성 유지.

---

### [LOW] xnew/xdelete 와 std::make_shared 가 혼용됨

**현재 상태**  
일부 코드는 `MakeShared<T>()`(커스텀 풀 삭제자)를, 다른 코드는 표준 `std::make_shared<T>()`를 사용합니다. 표준 `make_shared`는 풀을 거치지 않아 메모리 추적이 불완전합니다.

**개선 방향**  
`MakeShared`를 프로젝트 표준으로 고정하고, 표준 `make_shared` 직접 사용을 Lint 규칙으로 금지합니다. 또는 `MemoryStats::liveAllocCount`가 정확히 집계되도록 `new`/`make_shared` 오버로드를 global operator new로 통합합니다.

**예상 효과**  
메모리 통계 정확도 향상, 풀 효율 개선.

---

## 3. Job 시스템 / 동시성

### [HIGH] GameScene이 단일 JobQueue → 씬 1개인 경우 병렬 처리 불가

**현재 상태**  
`GameScene`은 `JobQueue`를 상속하므로 한 번에 하나의 Worker 스레드만 Update를 실행합니다. `sceneCount=1`이면 게임 전체 로직이 단일 스레드 직렬화됩니다.

**개선 방향**  
`sceneCount`를 늘려 Zone을 여러 씬에 분산합니다. 씬 간 경계 처리(`DoAsync`)는 이미 구현되어 있으므로, 설정값 조정만으로 수평 확장이 가능합니다.  
추가로, Zone 밀도가 높은 핫스팟 씬을 동적으로 분리하는 Scene Sharding 전략을 고려합니다.

**예상 효과**  
멀티코어 활용률 개선, 틱 지연 감소.

---

### [HIGH] JobQueue::Execute 에서 MAX_WORK_TICK(64) 초과 시 나머지 작업 지연

**현재 상태**  
`Execute()`는 최대 64개의 Job만 처리하고 나머지는 GlobalQueue로 반환합니다. 순간적으로 대량의 Job이 투입되면(예: 모든 몬스터 동시 사망) 처리 지연이 발생합니다.

**개선 방향**  
`LEndTickCount`(시간 기반 슬라이스)도 병행 체크하여 절대 시간 초과로 선점합니다. 또는 Job 우선순위 큐를 도입해 타이머 만료 Job을 일반 Job보다 먼저 처리합니다.

**예상 효과**  
순간 부하 시 타이머 정확도 유지.

---

### [MED] GlobalQueue 가 Lock 기반 Queue (LockQueue)

**현재 상태**  
`GlobalQueue`는 `LockQueue<JobQueueRef>`를 사용합니다. Worker 스레드 수가 많을수록 GlobalQueue 경합이 증가합니다.

**개선 방향**  
`std::atomic`과 CAS를 이용한 Lock-free MPMC 큐로 교체합니다. 또는 Worker 스레드별 Work-stealing Deque를 도입해 GlobalQueue 접근 빈도를 줄입니다.

**예상 효과**  
Worker 스레드 확장성(scalability) 개선.

---

### [MED] DoAsync 캡처 비용 — 람다가 매번 std::function 힙 할당

**현재 상태**  
`JobQueue::Push(Job job)`에서 `Job = std::function<void()>`를 큐에 push합니다. 캡처 크기가 SOO(Small Object Optimization) 한계(일반적으로 32~48B)를 초과하면 힙 할당이 발생합니다.

```cpp
GDBManager->DoAsync([dbId, hp, mp, exp, pos, yaw]() { ... });
// 6개 캡처 = 8+4+4+8+12+4 = 40B → SOO 초과 가능
```

**개선 방향**  
캡처 데이터를 구조체로 묶어 `xnew`로 할당 후 포인터만 캡처합니다. 또는 커스텀 Job 타입(타입 소거 없는 `std::packaged_task` 유사 구조)을 MemoryPool에서 할당합니다.

**예상 효과**  
Job 투입 비용 감소, 힙 단편화 완화.

---

## 4. GameServer 게임 로직

### [HIGH] 공격 판정 서버 신뢰 검증 미흡

**현재 상태**  
`HandleAttackHitDetection`에서 `attackPos`(클라이언트 전송값)를 그대로 사용합니다. 서버는 `attackPosToleranceSq(9.0f)` 설정값으로 허용 오차를 두지만, 현재 패킷 핸들러에서 이 검증이 실제로 수행되는지 확인이 필요합니다.

**개선 방향**  
`attackPos`를 서버의 `player->GetPos()` 기준으로 재계산하거나, 허용 오차 초과 시 패킷을 무시하고 경고 로그를 남깁니다. `attackPosToleranceSq` 검증 코드를 `HandleAttack` 진입부에서 명시적으로 수행합니다.

**예상 효과**  
위치 조작(텔레포트 어뷰징) 방어.

---

### [HIGH] Monster 리스폰 시 동일 MonsterRef 재사용 → 타이머 중복 위험

**현재 상태**  
`HandleMonsterDead` → `Reset()` → `EnterCreature(monster)` 흐름에서 동일한 `MonsterRef`를 재사용합니다. 만약 죽기 전에 `DoTimer` 콜백이 중복 등록되면 동일 몬스터가 두 번 Enter될 수 있습니다.

**개선 방향**  
`Monster`에 `_isScheduledForRespawn` 플래그를 추가하여 리스폰 타이머가 이미 예약된 경우 중복 등록을 방지합니다.

**예상 효과**  
몬스터 중복 스폰 버그 방지.

---

### [MED] World::_moveTable 메모리 크기

**현재 상태**  
`_moveTable`은 모든 (oldZone, newZone) 조합을 사전 계산합니다. `100 × 100 = 10,000` 조합에 대해 `MoveResult`(3개 Vector)를 저장합니다. 맵이 커질수록 O(N²) 메모리가 필요합니다.

**개선 방향**  
실제로 인접 Zone 간 이동만 일어나므로 `(oldId, newId)` 키를 `oldId의 인접 Zone으로 한정`하여 희소 맵(sparse map)으로 구성합니다. 또는 `IsTooFar` 판단 시 `nullptr`을 반환하는 현재 구조를 유지하되, 인접 Zone 목록에서만 MoveResult를 계산하여 테이블 크기를 O(N × 9)로 줄입니다.

**예상 효과**  
맵 확장 시 메모리 사용량 대폭 감소.

---

### [MED] Monster::Update가 GameScene 틱(50ms)에 종속

**현재 상태**  
`Monster::Update(deltaTime)`는 GameScene Update 루프에서 호출됩니다. 씬 틱이 지연되면 몬스터 AI도 같이 지연됩니다.

**개선 방향**  
몬스터 AI 업데이트를 별도 `JobTimer` 주기로 분리하거나, 활성 몬스터(타겟 있음)와 비활성 몬스터(IDLE/DEAD)의 업데이트 빈도를 다르게 설정합니다. 비활성 몬스터는 500ms 간격으로 탐색만 수행하면 충분합니다.

**예상 효과**  
틱 당 처리량 감소, CPU 사용률 개선.

---

### [MED] Zone::FillUpdatePacket 에서 패킷 flush 기준이 하드코딩

**현재 상태**  
spawn ≤30, despawn ≤400, move ≤100 기준이 Zone.h 템플릿 함수에 리터럴로 박혀 있습니다.

```cpp
if (++count >= 400) { flush(); count = 0; }   // despawn 상한
if (++count >= 30)  { flush(); count = 0; }   // spawn 상한
if (++count >= 100) { flush(); count = 0; }   // move 상한
```

**개선 방향**  
`CoreConfig.h`의 `Config::Zone` 네임스페이스에 상수로 이동합니다.

**예상 효과**  
설정 일원화, 매직 넘버 제거.

---

### [LOW] Player::_isSaveDirty 가 레벨업에만 적용

**현재 상태**  
`IsSaveDirty`는 레벨업 시에만 설정됩니다. 일반 이동·HP 변경 시 DB 저장은 `OnDisconnected`에서만 수행됩니다. 서버 비정상 종료 시 이동 데이터가 유실됩니다.

**개선 방향**  
주기적 자동 저장(예: 30초마다) `DoTimer`를 Player 입장 시 등록하거나, 이동 누적 거리 임계값 초과 시 저장을 트리거합니다.

**예상 효과**  
비정상 종료 시 플레이어 데이터 유실 최소화.

---

## 5. DB / Redis 레이어

### [HIGH] DBManager::DoAsync 가 JobQueue이므로 단일 스레드 DB 직렬화

**현재 상태**  
`DBManager`는 `JobQueue`를 상속하며, 모든 DB 쿼리는 GlobalQueue를 통해 단일 스레드에서 직렬 실행됩니다. 쿼리 지연이 누적되면 저장 큐가 쌓입니다.

**개선 방향**  
`DBConfig::threads` 설정값을 활용하여 DB Worker 스레드 풀을 운영합니다. 현재 `threads=2`로 설정되어 있지만 실제로 멀티스레드 DB 처리가 구현되어 있지 않습니다. `DbConnectionPool`의 연결 수를 스레드 수에 맞게 조정합니다.

**예상 효과**  
DB 처리 처리량 배증, 저장 지연 감소.

---

### [HIGH] Redis 연결 단일 인스턴스 (sw::redis::Redis)

**현재 상태**  
`RedisManager`는 단일 `std::unique_ptr<sw::redis::Redis>` 인스턴스를 사용합니다. Redis 명령이 직렬화되며, 네트워크 지연이 있는 경우 JobQueue 처리가 블로킹됩니다.

**개선 방향**  
`sw::redis::ConnectionPool` 또는 `RedisCluster`를 사용하여 연결 풀 기반으로 전환합니다. 또는 토큰 검증처럼 읽기 전용 명령은 비동기 파이프라인으로 배치 처리합니다.

**예상 효과**  
인증 처리량 향상, 로그인 레이턴시 감소.

---

### [MED] Redis 토큰 TTL(600초)이 하드코딩

**현재 상태**  
로그인 토큰 만료 시간이 `AccountController.cs`에 `TimeSpan.FromSeconds(600)`으로 고정되어 있습니다.

**개선 방향**  
`appsettings.json`에 `Auth:TokenTtlSeconds`로 외부화합니다.

**예상 효과**  
운영 환경별 TTL 조정 가능.

---

### [MED] SavePlayerInfo 와 SavePlayerLevelUp 의 중복 쿼리 로직

**현재 상태**  
두 메서드가 유사한 컬럼 세트를 UPDATE하며 코드가 중복됩니다.

**개선 방향**  
공통 `SavePlayer(dbId, level, hp, mp, exp, pos, yaw)` 내부 메서드로 통합하고, 레벨 파라미터 유무로 분기합니다.

**예상 효과**  
DB 쿼리 관리 단순화.

---

## 6. LoginWebServer (ASP.NET Core)

### [MED] AccountController에 비즈니스 로직 직접 위치

**현재 상태**  
토큰 생성·Redis 저장·서버 목록 조회 로직이 컨트롤러에 직접 작성되어 있습니다.

**개선 방향**  
`AuthService`, `ServerListService` 클래스로 분리하고 DI 컨테이너에 등록합니다. 컨트롤러는 입력 검증 및 응답 형식화만 담당합니다.

**예상 효과**  
단위 테스트 용이성 향상, 비즈니스 로직 재사용성 개선.

---

### [MED] CreateAccount 에서 중복 계정 처리가 예외 캐치 방식

**현재 상태**  
중복 계정을 `DbUpdateException`의 메시지 문자열 `"Duplicate"` 포함 여부로 판단합니다. DB 오류 메시지가 바뀌면 미탐지됩니다.

**개선 방향**  
`SaveChangesAsync` 전에 `AnyAsync(a => a.AccountName == req.AccountName)` 사전 체크를 추가합니다. 또는 MySQL 에러 코드(1062)를 직접 체크합니다.

**예상 효과**  
중복 감지 신뢰성 향상.

---

### [LOW] 비밀번호 검증 로직 없음 (길이·복잡도)

**현재 상태**  
`CreateAccount`에서 비밀번호 길이나 복잡도 제약이 없습니다.

**개선 방향**  
`FluentValidation` 또는 DataAnnotation으로 최소 길이(8자), 영숫자 혼합 요건을 추가합니다.

**예상 효과**  
취약한 비밀번호로 인한 보안 위험 감소.

---

## 7. 모니터링 / 운영

### [MED] ServerStats 의 DbStats 가 실제 측정되지 않음

**현재 상태**  
`DbStats::queryCount`, `queryTotalUs` 등이 선언되어 있지만 `DBManager`의 쿼리 실행 코드에서 실제로 집계하는 코드가 없습니다.

**개선 방향**  
`DbConnGuard` 소멸자 또는 `DbConnection::Execute` 래퍼에서 실행 시간을 측정하고 `GServerStats->db.*`에 기록합니다.

**예상 효과**  
DB 병목 실시간 가시성 확보.

---

### [MED] Logger 의 콘솔 출력 vs ServerStats 패널 동기화가 수동

**현재 상태**  
`Logger`와 `ServerStats`가 같은 콘솔을 공유하면서 `_consoleLock`과 ANSI 커서 이동으로 동기화합니다. 로그가 많으면 패널이 깨질 수 있습니다.

**개선 방향**  
터미널 UI 라이브러리(예: `ftxui`) 또는 로그 출력과 패널을 완전히 분리합니다. 또는 로그를 파일 전용으로 하고 콘솔은 패널 전용으로 사용합니다.

**예상 효과**  
콘솔 출력 안정성 향상.

---

### [LOW] 서버 종료 시 GameGlobal::Clear 에서 World 의 잔류 타이머 처리 불명확

**현재 상태**  
`GameGlobal::Clear()` → `CoreGlobal::Clear()` → `ThreadManager::Join()` 순서로 종료합니다. Worker 스레드가 Join되기 전에 `GameScene::Update` DoTimer가 재등록될 수 있고, JobTimer에 잔류 항목이 남을 수 있습니다.

**개선 방향**  
`_world->Stop()` 단계를 추가하여 GameScene의 자기 재등록 타이머를 중단하는 플래그(`_running`)를 설정한 후 스레드 Join합니다.

**예상 효과**  
깔끔한 서버 종료, 종료 시 크래시 방지.

---

## 8. 코드 품질 / 유지보수

### [MED] GameScene.h 의 _adjacentSceneGroups 가 매 BroadcastToAdjacentZones 마다 재생성

**현재 상태**  
```cpp
// GameScene.cpp
_adjacentSceneGroups.clear();
for (...) _adjacentSceneGroups[scene.get()].push_back(adjZone);
```
브로드캐스트마다 HashMap을 clear/rebuild합니다.

**개선 방향**  
Zone의 인접 씬 그룹을 초기화 시 사전 계산하여 `Zone` 또는 `World` 에 캐싱합니다.

**예상 효과**  
브로드캐스트 처리 속도 향상.

---

### [MED] GameObject::_nameUtf8 의 WString→string 매번 변환 방지는 잘 됐으나 Set 누락 가능성

**현재 상태**  
`SetName`에서 `_nameUtf8`을 함께 설정합니다. 그러나 `_name`을 직접 변경하는 코드가 있다면 불일치가 발생합니다.

**개선 방향**  
`_name`을 `private`로 두고 `SetName`을 통해서만 변경하도록 강제합니다. 현재 `protected`로 공개된 멤버들을 캡슐화합니다.

**예상 효과**  
이름 불일치 버그 방지.

---

### [LOW] CoreConfig.h 의 POOL_COUNT 계산이 직관적이지 않음

**현재 상태**  
```cpp
constexpr unsigned int POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256);
// = 32 + 8 + 8 = 48
```

**개선 방향**  
각 구간을 명시적 상수로 분리합니다:
```cpp
constexpr unsigned int POOL_SMALL_COUNT  = 1024 / 32;   // 32: 32~1024
constexpr unsigned int POOL_MEDIUM_COUNT = 1024 / 128;  // 128: 1025~2048
constexpr unsigned int POOL_LARGE_COUNT  = 2048 / 256;  // 256: 2049~4096
constexpr unsigned int POOL_COUNT = POOL_SMALL_COUNT + POOL_MEDIUM_COUNT + POOL_LARGE_COUNT;
```

**예상 효과**  
풀 구성 의도 명확화.

---

### [LOW] DummyClient 가 단일 세션만 테스트

**현재 상태**  
`DummyClient`는 하나의 세션만 생성합니다. 동시 접속 부하 테스트가 불가합니다.

**개선 방향**  
CLI 인자로 세션 수를 지정하고, 각 세션이 독립 `DummySession` + 별도 HTTP 로그인 토큰으로 동작하도록 확장합니다. `ClientService`의 `SessionFactory`를 활용하면 자연스럽게 N세션을 운용할 수 있습니다.

**예상 효과**  
현실적인 부하 테스트 가능.

---

## 개선 우선순위 요약

| 순위 | 항목 | 분류 |
|------|------|------|
| 1 | DBManager 단일 스레드 직렬화 → 스레드 풀 전환 | [HIGH] |
| 2 | 공격 위치 검증 강화 | [HIGH] |
| 3 | Monster 리스폰 중복 등록 방지 | [HIGH] |
| 4 | SendBufferManager Lock-free 전환 | [HIGH] |
| 5 | GameScene sceneCount 다중 씬 운용 | [HIGH] |
| 6 | Redis 연결 풀 전환 | [HIGH] |
| 7 | Player 주기적 자동 저장 | [LOW→MED] |
| 8 | DbStats 실제 집계 구현 | [MED] |
| 9 | Zone flush 상한 상수화 | [MED] |
| 10 | DummyClient 다중 세션 지원 | [LOW] |
