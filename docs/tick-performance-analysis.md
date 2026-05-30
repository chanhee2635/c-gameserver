# GameServer 틱 지연(Tick Lag) 분석

작성일: 2026-05-30

## 배경

클라이언트 2000명 동시 접속 시 GameServer의 게임 틱(`GameScene::Update`, 목표 50ms)이
크게 밀리는("튀는") 현상. 더미 클라이언트(공격자)로 부하를 주고 GameServer(방어자)의
상태 패널을 캡처해 측정.

## 테스트 환경

- 1 PC, localhost. CPU 16 logical cores.
- GameServer worker thread = `hardware_concurrency - 4` = **12**
- `sceneCount` = **4** (`server.json` / `world`)
- `updateTickMs` = 50 (목표 틱레이트 scene당 20/s × 4 = 80/s)
- 더미: 맵(1000×1000) 전체에 균일 랜덤 분포, 100ms마다 `C_MOVE` 전송
- 측정: GameServer stdout 패널을 파일로 캡처 후 `[TICKS]`/`[NETWORK]`/`[ENGINE]` 추출

## 측정 결과 (Before — 현재 코드)

| 지표 | idle ~ 550명 | **2000명** |
|---|---|---|
| AvgLag | 12ms | **44 ~ 110ms** |
| MaxLag | 13ms | **356 ~ 1606ms** (한 틱 최대 1.6초) |
| Scene 틱레이트 | 64 ~ 66 /s | **28 ~ 46 /s** (목표 80) |
| Out (송신) | ~0 | **6 ~ 11 MB/s, 16k ~ 23k pkt/s** |
| In (수신) | ~0 | 247 ~ 286 KB/s, 7 ~ 8k pkt/s |
| CPU | ~0% | **17 ~ 21%** (16코어 중 ≈ 3코어) |
| Job/s | 64 | 134 ~ 249 |
| IOCP/s | 0 | 22k ~ 30k |

## 진단

### 1) 상수 베이스라인 지연 12ms (idle ~ 550명)
- 부하와 무관하게 일정 → 컴퓨팅 비용이 아니라 **스케줄링 파이프라인 지연**.
- 경로: `JobTimer`(`GetTickCount64` 기반) → 워커 루프의 `IocpDispatch(timeout=10ms)` →
  `Distribute` → GlobalQueue → 워커 pop → `Update` 실행.
- 영향 경미. 우선순위 낮음.

### 2) 2000명 스파이크 (핵심)
- **CPU 17~21% (≈3코어)인데 틱이 1.6초까지 밀림** → CPU 부족이 아님.
  16코어/12워커 중 대부분이 놀고 있음.
- 원인: 게임 로직이 **scene 큐 4개**에서만 돌고, 각 `GameScene::Update()`가
  이동처리 + zone변경 + **BroadcastScene(직렬화 + 전 플레이어 전송)** 을
  **하나의 거대한 단일 Job**으로 수행. 2000명이면 한 Update가 워커 1개를
  최대 1.6초 독점 → 다음 틱이 그만큼 지연.
- 즉 **"4 scene × 단일 Job 직렬화"가 병목.** 송신 fan-out(Out 10MB/s)이 비용의 대부분.

### 지연의 두 성분
- **지속 44~110ms**: BroadcastScene fan-out (정상 부하 송신량).
- **순간 1606ms**: 로그인 버스트(+500 동시 EnterCreature → 이웃 9 zone spawn 패킷 폭풍).

## 개선 후보 (효과순)

1. **Update의 broadcast(전송)를 틱 임계경로에서 분리 → 워커 풀로 병렬화** *(이번에 적용)*
   - Update(scene 스레드)는 계산 + 직렬화만, 실제 per-player 전송은 별도 lane으로
     offload. `Session::Send`는 thread-safe라 안전. 노는 12워커 활용.
2. scene 병렬도 ↑ (`sceneCount` 4 → 8/16). 단 cross-scene 경계 부하 증가.
3. Out 볼륨 감축: 이동 브로드캐스트 주기 ↑(50→100ms), 위치 양자화/델타, AOI 축소.
4. 로그인 버스트 완화: EnterCreature 스태거링 / spawn 패킷 점진 전송.

### 공격자(더미) 관점 참고
- 더미는 맵 전역 균일 랜덤 → 클러스터링이 거의 없는 **낙관적** 부하.
  실유저는 마을/보스에 몰려 한 zone fan-out이 훨씬 커짐 → 실전은 더 심함.
- 더미 측도 2000에서 연결 churn(10054 다수). 한 PC에서 3서버+2000더미 동시 구동의
  한계가 일부 섞임.

---

## 개선 #1 적용: 브로드캐스트 전송 offload (send-lane 풀)

### 변경 내용
- `GameScene::BroadcastScene`의 per-player 전송을 scene 스레드에서 직접 하지 않고,
  zone 플레이어를 스냅샷(scene 스레드, zone 읽기 안전)한 뒤 **send-lane 풀(8개 JobQueue)**
  로 offload. 실제 `player->Send`는 기존 12워커가 lane을 pop해 **병렬 처리**.
- 같은 zone은 항상 같은 lane(`zoneId % 8`)으로 보내 그 zone의 패킷 순서를 보존.
- `Session::Send`는 thread-safe, lane job이 `PlayerRef`/`SendBufferRef`를 잡아 수명 안전.
- 파일: `GameScene.cpp` (`OffloadBroadcast`, send-lane 풀), `GameScene.h`.

### 측정 결과 (After — 동일 조건 2000명)

| 지표 | Before (2000) | **After #1 (2000)** | 변화 |
|---|---|---|---|
| AvgLag | 44 ~ 110ms | **15 ~ 54ms** | ▼ 약 2~4배 개선 |
| MaxLag | 356 ~ 1606ms | **59 ~ 482ms** | ▼ 최악 1.6초 → ~0.5초 |
| Scene 틱레이트 | 28 ~ 46 /s | **43 ~ 61 /s** | ▲ 목표 80에 근접 |
| CPU | 17 ~ 21% | **26 ~ 39%** | ▲ 노는 코어 활용 시작 |
| Job/s | 134 ~ 249 | **8,000 ~ 10,700** | 전송이 병렬 Job으로 분산됨 |
| Out | 6 ~ 11 MB/s | 8 ~ 13 MB/s | ≈ 동일(같은 데이터, 경로만 병렬) |

### 결론: **개선 성공 → 변경 유지**
- 틱 지연이 평균 2~4배, 최악 스파이크가 1.6초 → ~0.5초로 크게 감소.
- CPU 사용률이 17→35% 부근으로 올라 **그동안 놀던 워커/코어를 활용**(병목 완화 확인).
- `Job/s`가 ~200 → ~9,000으로 급증 = 전송이 scene 스레드 인라인에서 lane 병렬 Job으로
  옮겨간 직접 증거.

### 남은 한계 / 다음 후보
- 여전히 간헐 MaxLag ~480ms 스파이크 → **로그인 버스트**(`SendSpawnPacketsToPlayer`가
  아직 scene 스레드 동기) 영향. 개선 #4(버스트 완화)로 추가 공략 가능.
- 지속 AvgLag도 목표(50ms 틱)엔 아직 여유 부족 → 개선 #2(scene 병렬도) / #3(Out 볼륨 감축)
  병행 시 추가 효과 예상.
