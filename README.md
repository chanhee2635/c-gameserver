# C++ IOCP 멀티플레이 게임 서버

> IOCP 기반 비동기 네트워크와 JobQueue 패턴으로 구현한 멀티플레이 게임 서버 포트폴리오입니다.

▶ [시연 영상 (YouTube)](https://youtu.be/dGX39CQrB1o)

---

## 주요 구현 내용

- **IOCP 비동기 네트워크** — AcceptEx 선발급, Worker Thread 기반 Recv/Send 처리
- **JobQueue 비동기 처리** — World → GameScene(×4) → Zone(5×5) 계층 분산 구조
- **이동 동기화** — 100ms 틱 기반 C_Move 패킷 처리, Zone 경계 ZoneChange 처리
- **몬스터 AI** — Recast/Detour NavMesh 경로탐색, Idle/Moving/Attack/Dead 상태머신
- **로그인 인증** — Login Server 분리, Redis UUID 토큰 발급, 게임 서버 목록 제공
- **채팅 시스템** — Chat Server 분리, Redis Pub/Sub 기반 일반/서버 채팅
- **200명 동시접속** — DummyClient 부하 테스트 검증

---

## 서버 구조
```
Login Server (:7778)  —  인증 · 서버 목록
Game Server  (:7777)  —  게임 로직 · 이동 동기화 · 몬스터 AI
Chat Server  (:7779)  —  채팅 · Redis Pub/Sub
```

---

## 기술 스택

| 구분 | 기술 |
|---|---|
| Server | C++ 17, IOCP (Windows), Google Protobuf, MySQL ODBC, Redis (hiredis) |
| Client | Unity (C#), Protobuf-net |
| AI | Recast/Detour NavMesh |
| 패턴 | IOCP 비동기 I/O, JobQueue, ObjectPool, Zone 기반 분산 |

---

## 트러블슈팅

### HandleZoneChange Race Condition
- **문제** : 씬 경계 통과 시 200명 전체 이동 정지. 서버 로그에 에러가 없어 원인 파악이 어려웠음
- **원인** : `SetGameScene(newScene)` 즉시 교체 → 이후 패킷이 newScene으로 라우팅 → `_players.find()` 실패 → MoveJob 전부 드롭
- **해결** : `DoAsyncPush` 람다 안에서 `newZone->Enter()` 완료 후 `SetGameScene` 교체

### MoveJob 즉시 처리로 인한 JobQueue 적체
- **문제** : 200명 밀집 시 초당 수천 개의 MoveJob이 쌓여 서버 틱이 밀리는 현상
- **원인** : 패킷 수신 즉시 JobQueue에 Push → Worker Thread 2개로 처리 한계 초과
- **해결** : LockQueue에 누적 후 100ms 틱마다 PopAll 일괄 처리, 중복 Job은 최신 것만 반영

---

## 빌드 환경

- Visual Studio 2022
- Windows SDK

## 외부 라이브러리 (별도 설치 필요)

| 라이브러리 | 용도 | 링크 |
|---|---|---|
| Google Protobuf | 패킷 직렬화 | https://github.com/protocolbuffers/protobuf |
| hiredis | Redis 클라이언트 | https://github.com/redis/hiredis |
| MySQL ODBC | DB 연결 | https://dev.mysql.com/downloads/connector/odbc/ |
| Recast/Detour | NavMesh | https://github.com/recastnavigation/recastnavigation |

`Libraries/Include/` 경로에 각 라이브러리 헤더를 배치 후 빌드하세요.
