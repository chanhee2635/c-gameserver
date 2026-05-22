# AOI 이동/공격 패킷 파이프라인 & 단계별 최적화

> 클라이언트 이동/공격 패킷이 들어왔을 때의 처리 파이프라인과, 대규모 밀집(핫스팟) 부하를 막기 위한 최적화 기법을 **"순진한 구현 → 문제 발견 → 보완"** 순서로 정리한 문서.
> 이 진화 과정 자체가 기술 면접에서 사고력을 보여주는 핵심 무기다.
>
> ⚠️ 코드는 개념 설명용 의사코드(`GameRoom`/`Sector` 용어)다. 실제 이 프로젝트는 `GameScene`/`Zone`/`MoveResult` 구조를 쓰므로 매핑은 문서 하단 [실제 코드와의 매핑](#부록-실제-코드와의-매핑) 참고.

---

## 0. 대원칙: 네트워크 스레드는 "던지기만", 로직 스레드가 "처리"

> 네트워크 워커 스레드는 패킷을 파싱해서 큐에 넣기만 하고(Non-blocking), 실제 데이터 연산은 싱글 스레드처럼 작동하는 로직 스레드(JobQueue)가 처리한다. → **레이스 컨디션 원천 차단.**

```
[클라 인풋] → [네트워크 스레드] ──(PushJob)──> [GameRoom JobQueue] → [로직 스레드가 동기 실행]
```

---

## 1. 기본 파이프라인: C_MOVE 수신 → JobQueue → GameRoom

```cpp
// 1단계: 네트워크 스레드 (PacketHandler.cpp)
void PacketHandler::Handle_C_MOVE(SessionRef& session, BYTE* buffer, int32 len)
{
    Protocol::C_MOVE pkt;
    if (!pkt.ParseFromArray(buffer, len)) return;       // 역직렬화

    auto gameSession = static_pointer_cast<GameSession>(session);
    PlayerRef player = gameSession->GetPlayer();
    if (player == nullptr) return;

    Vector3 newPos = { pkt.pos().x(), pkt.pos().y(), pkt.pos().z() };

    // ★ JobQueue에 함수 예약 → 네트워크 스레드는 즉시 빠져나가 다음 I/O 처리
    GGameRoom->PushJob(&GameRoom::HandleMove, player, newPos);
}

// 2단계: 로직 스레드 (GameRoom.cpp) — FlushJob() 시 동기 실행
void GameRoom::HandleMove(PlayerRef player, Vector3 newPos)
{
    if (player == nullptr) return;
    // (선택) 핵 방지: 좌표가 너무 튀었는지 / 벽을 뚫었는지 검증
    // if (!IsValidMove(player->GetPosition(), newPos)) return;

    SectorIndex oldIndex = player->GetCurrentSector();
    SectorIndex newIndex = ConvertToSector(newPos);
    player->set_position(newPos);

    if (oldIndex == newIndex)
        BroadcastMoveToNearby(newIndex, player);   // 같은 섹터 → 단순 S_MOVE
    else
        UpdateSectorAndVision(player, oldIndex, newIndex);  // 경계 넘음 → 시야 교체
}
```

---

## 2. 격자 경계 이동 시 시야 교체 (AOI 정밀 필터링)

섹터가 바뀌면 단순 브로드캐스트가 아니라 **시야 교체**가 필요하다.

- **내가 멀어진 섹터의 유저:** 서로 `S_DESPAWN`
- **내가 새로 진입한 섹터의 유저:** 서로 `S_SPAWN`
- **유지되는 교집합 섹터:** `S_MOVE`만

```cpp
void GameRoom::UpdateSectorAndVision(PlayerRef player, SectorIndex oldIndex, SectorIndex newIndex)
{
    _sectors[oldIndex.x][oldIndex.z].RemoveObject(player);   // 소속 섹터 교체
    _sectors[newIndex.x][newIndex.z].AddObject(player);
    player->SetCurrentSector(newIndex);

    // 기존 3x3과 새 3x3을 대조하여 Spawn/Despawn 대상 선별
    // - old 3x3 중 new 3x3에 없는 칸 → Despawn
    // - new 3x3 중 old 3x3에 없는 칸 → Spawn
    // - 겹치는 칸 → Move
    // (3x3 = 9칸 고정이므로 이 대조 자체는 O(1))
}
```

> **면접 단골 질문:** "오브젝트가 많아지면 3x3 안의 전수 대조가 부담되지 않나요?"
> → 그렇다. 이게 다음 장의 **O(N²) 문제**다.

---

## 3. O(N²) 문제와 3가지 해결책

### 문제의 본질

> 내가 한 걸음 움직일 때마다 운동장의 모든 사람(N명)을 붙잡고 거리 대조 → 1,000명이 동시에 움직이면 `1,000 × 1,000 = 1,000,000`번 연산이 매 프레임 폭발.

핫스팟(밀집)에서 한 섹터에 유저가 몰리면 섹터 기반 AOI도 결국 섹터 내 O(N²) 패킷 복사·연산으로 서버가 굳는다.

### ① 시야 갱신 틱(Hz) 분리 — [가성비 최강, 우선 추천]

> 💡 비유: **위치 동기화는 카톡(실시간), 시야 검사는 우편물(주기적).**

인간의 눈은 0.1~0.3초 시야 지연을 못 느낀다. → 무거운 Spawn/Despawn 연산만 200~500ms 주기로 분리.

```cpp
void GameRoom::HandleMove(PlayerRef player, Vector3 newPos)
{
    player->SetPosition(newPos);
    AnnounceMove(player);   // 단순 이동은 즉시 토스 (가벼움)

    uint64 currentTick = GetTickCount64();
    if (currentTick - player->GetLastVisionTick() >= 200)   // 200ms마다만
    {
        player->SetLastVisionTick(currentTick);
        UpdateSectorAndVision(player);   // 비싼 9칸 대조는 여기서만!
    }
}
```

- **효과:** 매 프레임 O(N²)가 초당 5회로 감소 → CPU 70~80% 절감.
- **부작용:** 빠르게 이동 시 새 유저가 0.2초 늦게 스폰(Pop-in). → 서버 시야 마진 + 클라 Fade-in으로 보완 (5장).

### ② 2D List-Based AOI (Sweep and Prune)

> 💡 비유: **키 순서로 줄 세워두고, 내 주변만 앞뒤로 훑다가 키 차이 크면 포기.**

공간을 격자로 나누는 대신 모든 오브젝트를 X축/Z축 정렬 리스트로 유지.

```
[X축 리스트] -- (A:50) -- (B:80) -- [나:100] -- (C:115) -- (D:128) -- |시야벽:130| -- (E:150) ...
                                      ↳ 탐색 시작 → C(OK) → D(OK) → E(시야 밖! 즉시 break)
```

- **정렬 유지:** 플레이어는 직전 프레임과 순서가 거의 안 바뀌므로 삽입정렬 비용이 O(N)에 수렴.
- **Early Exit:** 내 시야(30m)를 벗어나는 오브젝트를 만나는 순간 탐색 중단.
- **효과:** 광장에 1,000명이 뭉쳐도 실제 시야 내 15명만 스캔하고 끝. 밀집 지역 전수조사 완전 회피.

### ③ Dynamic Quad-Tree (동적 공간 분할)

> 💡 비유: **사람 많아지면 행정구역을 도 → 시 → 동으로 잘게 쪼개기.**

```
[유저 없을 때]          [광장에 몰릴 때]
+-------------+        +------+------+
|             |        |      |  |   |
|  거대 노드   |   =>   |  1   +--+--+
|  (인원 적음) |        |      |  |   |
+-------------+        +------+------+
                        (2번 구역만 4개로 split)
```

- 노드당 최대 인원(예: 30명) 초과 시 4분할(Split), 적어지면 병합(Merge).
- **효과:** 밀집 노드만 좁게 탐색 → 전체 대조 O(N)에서 트리 탐색 O(log N)으로 감소.
- **단점:** 동적 이동 잦으면 split/merge 비용 발생. (고정 그리드 대비 구현 복잡)

| 기법 | 핵심 | 난이도 | 추천도 |
|---|---|---|---|
| ① 시야 갱신 틱 분리 | 연산 주기를 낮춤 | ★ | 최우선 (가성비) |
| ② Sweep and Prune | 정렬 + Early Exit | ★★ | 밀집 회피 강력 |
| ③ Quad-Tree | 동적 공간 분할 | ★★★ | 밀집 편차 클 때 |

---

## 4. 인원 제한 + 우선순위 필터링 (Max Broadcasters)

알고리즘을 O(log N)으로 줄여도, 한 화면에 300명이 동시에 스킬 쓰면 `300×300 = 90,000` 패킷의 **물리적 한계**가 존재. → 컨텐츠 레벨 컷트라인.

> "클라이언트에게 전송하는 주변 오브젝트는 **최대 40~50명**으로 제한한다."

**우선순위 필터링** (단순 거리순으로 자르면 안 됨):
1. **1순위:** 같은 파티/길드원 (거리 무관 100% 보임)
2. **2순위:** 전투 중이거나 나를 타겟팅한 적/몬스터
3. **3순위:** 그 외 일반 유저 (여기서부터 거리순 컷)

### 구조: `_myViewport`(내 시야 명단) + 200ms 주기 갱신

```cpp
class Player : public Object
{
public:
    uint64 GetLastVisionTick() const { return _lastVisionTick; }
    void   SetLastVisionTick(uint64 tick) { _lastVisionTick = tick; }
    std::set<uint64>& GetMyViewport() { return _myViewport; }  // 현재 인지 중인 대상 (최대 40)
private:
    uint64 _lastVisionTick = 0;
    std::set<uint64> _myViewport;
};
```

이동/공격 패킷은 9칸 전수조사 없이 **`_myViewport`의 검증된 40명에게만** 전송 → 인당 루프가 최대 40회로 고정(O(1)에 수렴).

```cpp
void GameRoom::HandleMove(PlayerRef player, Vector3 newPos)
{
    player->SetPosition(newPos);
    auto sendBuffer = PacketHandler::MakeSendBuffer(/* S_MOVE */);

    for (uint64 targetId : player->GetMyViewport())  // 1,000명 있어도 최대 40회
    {
        ObjectRef target = _objects[targetId];
        if (target && target->GetSession())
            target->GetSession()->Send(sendBuffer);
    }
}
```

### `_myViewport` 갱신 (200ms 주기 `UpdateRoomVisions`)

```cpp
void GameRoom::Update()  // 메인 로직 루프
{
    uint64 now = GetTickCount64();
    if (now - _lastRoomVisionTick >= 200) {
        _lastRoomVisionTick = now;
        UpdateRoomVisions();   // 방 내 모든 플레이어의 _myViewport 일괄 갱신
    }
}

void GameRoom::UpdateRoomVisions()
{
    for (auto& [objectId, obj] : _objects)
    {
        if (obj->GetObjectType() != GameObjectType::Player) continue;
        PlayerRef player = static_pointer_cast<Player>(obj);

        // 1) 주변 9칸 후보군 수집
        std::vector<ObjectRef> candidates;
        SectorIndex s = player->GetCurrentSector();
        for (int32 dx=-1; dx<=1; ++dx)
          for (int32 dz=-1; dz<=1; ++dz)
            for (auto& n : _sectors[s.x+dx][s.z+dz].GetObjects())
              if (n->GetObjectId() != player->GetObjectId())
                candidates.push_back(n);

        // 2) 우선순위 정렬 (파티원 우선 → 거리순)
        std::sort(candidates.begin(), candidates.end(), [player](auto& a, auto& b){
            float dA = Vector3::DistanceSq(player->GetPosition(), a->GetPosition());
            float dB = Vector3::DistanceSq(player->GetPosition(), b->GetPosition());
            return dA < dB;
        });

        // 3) 컷트라인 40명 → newViewport
        std::set<uint64> newViewport;
        for (int32 c = 0; c < (int32)candidates.size() && c < 40; ++c)
            newViewport.insert(candidates[c]->GetObjectId());

        // 4) 기존 vs 신규 대조 → Spawn/Despawn (세트 대조)
        for (uint64 oldId : player->GetMyViewport())
            if (newViewport.count(oldId) == 0) {
                // Despawn: 나에게서 지우고, 상대 뷰포트에서도 나를 즉시 제거 (상호 동기화)
            }
        for (uint64 newId : newViewport)
            if (player->GetMyViewport().count(newId) == 0) {
                // Spawn: 나에게 생성하고, 상대 뷰포트에도 나를 강제 편입 (상호 동기화)
            }

        // 5) 스냅샷 교체
        player->GetMyViewport() = std::move(newViewport);
    }
}
```

> **왜 상대방 뷰포트도 같이 수정하나?** 네트워크 동기화는 항상 **상호작용**이다. 내 화면에 A가 스폰됐으면 A 화면에도 내가 스폰돼야 한다. 한쪽만 갱신하면 A의 시야 틱이 올 때까지 **시야 불일치**가 생긴다. → 한쪽이 시야 연산할 때 상대 명단까지 세트로 맞춰버린다.

---

## 5. Pop-in 문제와 해결 (인원 제한의 부작용)

> **문제:** 41등이던 유저가 40등 안으로 들어오는 순간 화면에 **툭! 순간이동(Pop-in)**. 경계선(40~41등)에 선 유저는 한 걸음마다 스폰/디스폰 무한 반복(깜빡임).

### ① 클라이언트: 알파 블렌딩 (Fade-In/Out)

스폰 시 즉시 렌더링 대신 0.3초간 투명도(Alpha 0→1)로 스르륵 등장. → 유저는 "안개 속에서 걸어 들어왔구나"로 자연스럽게 인지.

### ② 서버: "두 개의 링" 구조 (Hysteresis Buffer) — ★ 핵심

스폰 컷과 디스폰 컷을 **다르게** 설정:

```
[ 나 ] ─── (1~40등: 무조건 보임) ─── (41~60등: 기존에 보이던 애만 유지) ─── (61등~: 블라인드)
```

- **진입(Spawn):** 선착순 40명까지만 새로 시야에 들임.
- **유지/퇴출(Despawn):** 이미 들어온 유저는 **60명 밖으로 밀려나기 전까지** 억지로 유지.
- → 경계선에서 순위가 40 안팎으로 진동해도 깜빡임 방어. (앞서 다룬 그리드 마진 버퍼와 동일 원리)

### ③ 컨텐츠: 레이어 분리 / 채널링

- **채널링:** 광장 인원 폭발 시 '1채널/2채널'로 세션 분산.
- **중요 레이어 제외:** 파티원/길드원/타겟 보스는 40명 컷 계산에서 아예 제외(0순위 통과). 이름 모를 행인만 선착순 컷.

---

## 6. 밀집 테스트는 화면으로 불가능

> 시야를 40명으로 제한하면 **내 화면만 봐선 "렉 없네? 성공!"으로 착각**하기 쉽다. 화면으로는 1,000명 밀집의 진짜 부하를 절대 못 본다. → 서버의 '속살'을 까는 3가지.

1. **더미 클라이언트 봇:** 그래픽 없는 C++ 콘솔로 가짜 유저 1,000개를 한 좌표에 몰아넣고 이동/공격 패킷 폭사. 진짜 클라 1개를 광장에 세워두고 **서버 CPU/레이턴시**를 관측.
2. **서버 내부 메트릭:** 무거운 연산 구간에 타이머 삽입.
   ```cpp
   auto start = std::chrono::high_resolution_clock::now();
   // ... 1,000명 대조 + 40명 컷 ...
   auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                 std::chrono::high_resolution_clock::now() - start).count();
   if (us > 5000) LOG_WARN("Vision Update Overload: %lld us", us);  // 5ms 초과 경고
   ```
   화면은 60fps여도 콘솔에 Overload 경고가 쏟아지면 → '40명 컷 이전, 후보 1,000명 수집/정렬' 단계에서 CPU가 비명.
3. **네트워크 대역폭(PPS):** 주변에 1,000명이 와도 내가 받는 패킷이 **40명분에서 수평(Plateau) 유지**되면 성공. 1,000명분이 다 들어와 그래프가 수직 상승하면 인원 제한에 구멍.

---

## 7. Tick-based Movement Batching (이동 모아 처리)

> 이동 패킷이 올 때마다 즉시 처리(O(1)이라도 초당 수만 번이면 컨텍스트 스위칭/캐시 미스로 서버 뻗음)하는 대신, **요청은 큐에 쌓아만 두고 50~100ms 틱에 일괄 처리.**

```cpp
struct MoveRequest { Vector3 pos; };

class Player : public Object {
public:
    void PushMoveRequest(const Vector3& pos) {            // 네트워크 스레드: 가볍게 쌓기만
        std::lock_guard<std::mutex> lock(_moveLock);
        _moveQueue.push(MoveRequest{ pos });
    }
    bool PopLatestMoveRequest(MoveRequest& out) {         // 로직 틱: 최신 좌표만 사용
        std::lock_guard<std::mutex> lock(_moveLock);
        if (_moveQueue.empty()) return false;
        while (!_moveQueue.empty()) { out = _moveQueue.front(); _moveQueue.pop(); }
        return true;   // ★ 50ms 사이 3번 들어와도 중간 과정 버리고 마지막만!
    }
private:
    std::mutex _moveLock;
    std::queue<MoveRequest> _moveQueue;
};
```

```cpp
// 네트워크 스레드: JobQueue도 안 거치고 플레이어 큐에 바로 Push
void PacketHandler::Handle_C_MOVE(SessionRef& session, BYTE* buffer, int32 len) {
    /* parse... */
    player->PushMoveRequest(newPos);
}

// 50ms 주기 일괄 처리
void GameRoom::UpdateMoveTick() {
    for (auto& [id, obj] : _objects) {
        PlayerRef player = /* cast */;
        MoveRequest req;
        if (!player->PopLatestMoveRequest(req)) continue;  // 이번 틱 이동 없으면 패스
        player->SetPosition(req.pos);
        auto buf = PacketHandler::MakeSendBuffer(/* S_MOVE, 패킷 1회만 빌드 */);
        for (uint64 tid : player->GetMyViewport())         // 뷰포트(40명)에게만
            if (auto t = _objects[tid]) if (auto s = t->GetSession()) s->Send(buf);
    }
}
```

- **장점:** 매크로/핵으로 초당 100번 쏴도 서버는 50ms에 1번만 연산 → **어뷰징 면역**. 소켓 시스템 콜 횟수 급감.
- **단점:** 상대 캐릭터가 0.1초마다 뚝뚝 끊겨 보임. → 클라가 **데드 레커닝(Dead Reckoning) + 보간(Interpolation)**으로 미래 위치 예측해 부드럽게 커버.

> ⚠️ **주의:** 틱 배칭을 해도 "50m 시야 **전원**"에게 보내면 O(N²) 트래픽은 그대로다. 300명 밀집 시 `300×300×20회 = 180만 패킷/초` → OS 송신 버퍼 폭발. **반드시 인원 제한과 함께** 써야 한다 (`300×40×20 = 24만`, 약 86% 감소).

---

## 8. Packet Bundling (List 전송)

> 이번 틱에 움직인 수십 명의 좌표를 **하나의 통짜 리스트 패킷**에 담아 한 번에 Send. (= Packet Bundling)

```
[개별 전송]  [헤더|A][헤더|B][헤더|C]...   (헤더 오버헤드 N배 + Send N번)
[번들 전송]  [헤더 | 개수(N) | A | B | C ...]   (헤더 1개 + Send 1번!)
```

- **장점:** 헤더 오버헤드 증발 + 소켓 시스템 콜 1회로 압축.
- **함정:** 유저마다 시야가 달라 **"나만의 커스텀 리스트"를 개별 직렬화**해야 함 → 300명이면 300번 `SerializeToArray()`. 번들링해도 **직렬화 O(N²)는 안 줄고 오히려 무거워질 수** 있음. + 클라가 매 틱 300명 좌표 파싱 → 모바일 발열/프레임 드랍.
- **결론:** 리스트 최대 크기를 **`_myViewport`(40명)로 제한**해야 완성.

```cpp
void GameRoom::UpdateMoveTick_BundleVersion() {
    for (auto& [id, obj] : _objects) {
        PlayerRef player = /* cast */;
        Protocol::S_MOVE_LIST listPkt;
        for (uint64 tid : player->GetMyViewport()) {       // 40명 중
            ObjectRef t = _objects[tid];
            if (t && t->IsMovedThisTick()) {               // 이번 틱에 움직인 자만
                auto m = listPkt.add_moves();
                m->set_object_id(tid);
                /* m->mutable_pos() = t->pos ... */
            }
        }
        if (listPkt.moves_size() > 0)                      // 묶어서 단 1번 Send
            player->GetSession()->Send(PacketHandler::MakeSendBuffer(listPkt));
    }
}
```

---

## 9. Grid 단위 통짜 전송 (직렬화 O(N)화) — 발상 전환

> 유저마다 시야가 달라서 O(N²) 직렬화가 터지는 거라면, **시야 계산을 버리고** 월드를 격자로 잘게 쪼개 **격자별 리스트를 1번만 빌드해 그 칸 전원에게 같은 버퍼를 복사 전송**하자. (= Grid-based Interest Management / Spatial Hashing)

```
[1단계] Grid[5,3] 칸에서 이번 틱에 움직인 15명 → Protobuf 리스트 1번만 Serialize (메모리에 1번)
[2단계] Grid[5,3]에 선 유저 50명 → 이미 구운 SendBuffer를 그대로 복사해서 50명에게 Send
```

- **효과:** `SerializeToArray()` 호출 횟수가 **유저 수가 아니라 격자 수에 비례**. 유저 1만 명이어도 같은 칸은 동일 버퍼 공유 → 서버 CPU 거의 해방.

### 함정: 격자 경계선 = 투명 벽 현상

내 칸만 보내면, 경계선 바로 너머(이웃 칸) 유저가 코앞인데도 안 보이다가 선을 넘는 순간 툭 튀어나옴.

### 해결: 주변 9칸(9-Grid) 브로드캐스트

```
┌───┬───┬───┐
│ ↖ │ ↑ │ ↗ │   서버는 매 틱 격자별 이동 패킷을 1번만 빌드해 들고 있다가,
├───┼───┼───┤   유저에게는 그가 속한 칸 + 이웃 8칸 = 총 9개의
│ ← │ 나│ → │   '이미 구워진 SendBuffer'를 집어서 그대로 던진다.
├───┼───┼───┤
│ ↙ │ ↓ │ ↘ │
└───┴───┴───┘
```

→ 개별 시야 계산(O(N²)) 없고, 직렬화는 격자당 1번, 경계 너머 유저까지 부드럽게 보임.

### 비교: 반지름 시야 제한 vs 9칸 격자 전송

| 항목 | 반지름 시야 제한 | 9칸 격자 통짜 전송 |
|---|---|---|
| 서버 CPU (직렬화) | ❌ 유저 수만큼 직렬화 | ⭕ 격자 수만큼만 → 압도적 가벼움 |
| 서버 CPU (시야 판정) | ❌ 매번 거리계산/정렬 | ⭕ 인덱스 연산(x/격자크기) O(1) |
| 네트워크 트래픽 | ⭕ 필요한 40명만 컷 | ❌ 9칸에 200명 몰리면 200명분 다 들어옴 |
| 주 장르 | 3D 백뷰 MMO (와우, 리니지2M) | 2D/쿼터뷰 대규모, 모바일 |

→ 격자 9칸 방식은 서버 CPU를 거의 해방하지만, **공성전 핫스팟에선 트래픽 폭탄** 위험. 격자 크기(10~15m)를 기획상 최대 밀집도와 타협해 튜닝해야 함.

---

## 10. 우선순위 기반 분할 전송 (Traffic Throttling) — 끝판왕

> 9칸 안에 500명이 몰려 트래픽이 터지면, 그 통짜 패킷마저 **여러 개로 쪼개 순차 전송**(Slicing + Round-Robin).

```
1틱(0ms):   [1~100번] 이동 전송
2틱(50ms):  [101~200번] 전송
3틱(100ms): [201~300번] 전송
```

- **장점:** 대역폭 스파이크 제거, 트래픽이 시냇물처럼 고르게 분산 → 버퍼 오버플로/드랍 방지.
- **단순 번호순 분할의 치명적 문제:** 나를 때리러 오는 적의 갱신이 밀려 **동기화 붕괴**(내 화면엔 멀리 있는데 서버에선 이미 칼 맞는 중).

### 해결: 우선순위 레이어로 쪼개기 (Priority Split)

```
[9칸 격자 이동 패킷 빌드]
 ├── 레이어 A (최상위): 전투 중인 적 / 파티원 / 화면 중앙 → 50ms마다 무조건 전송
 ├── 레이어 B (중위):   멀리 걸어가는 아군            → 100ms마다 격주 전송
 └── 레이어 C (하위):   구석 NPC / 잠수 유저          → 200ms마다 가끔 전송
```

- 서버 CPU: 격자별로 A/B/C 패킷 3개만 구우면 됨 → 여전히 널널.
- 트래픽: 덜 중요한 유저 전송 주기를 뒤로 밀어(Throttling) 총량 통제.
- 체감: 파티원/적(A)은 50ms 칼동기화 → 유저는 렉 무체감. 배경 행인만 살짝 끊김.

> **삼위일체 완성:** `[격자 기반 O(1) 시야] + [서버 1회 직렬화 공유] + [우선순위 대역폭 제어]` → 단일 서버로 수천 명 공성전을 견디는 하드코어 MMO 아키텍처.

---

## 면접 답변 스크립트 모음

**Q. 섹터 안에 유저 몰리면 O(N²) 부하 생기는데?**
> "고정 격자는 핫스팟에서 섹터 내 전수조사로 부하가 심해집니다. 보완책으로 ① 시야 갱신 주기를 200~300ms로 분리해 O(N²) 연산 빈도를 낮추고, ② 'Sweep and Prune'으로 축별 정렬 리스트에서 시야 밖을 만나면 즉시 Early Exit하며, ③ 패킷 레이어에서 '인당 최대 시야 오브젝트 수'를 40명 내외로 제한해 로직 스레드의 패킷 복사 오버헤드를 물리적으로 가둡니다."

**Q. 40명 제한하면 경계선 유저가 Pop-in 되지 않나?**
> "맞습니다. 이를 막기 위해 서버에서 스폰/디스폰 등수 마진을 다르게 주는 Hysteresis 필터링을 적용해 깜빡임을 방어하고, 클라와 협업해 신규 스폰 캐릭터에 0.3초 Fade-In(알파 블렌딩)을 넣어 시각적 이질감을 감춥니다."

**Q. 이동을 즉시 처리 안 하고 틱으로 모으는 이유는?**
> "네트워크 스레드는 플레이어 큐에 요청만 쌓고, 메인 스레드가 50ms 주기로 돌며 중간 과정 패킷은 드롭한 채 최신 좌표만 갱신합니다. 불필요한 연산과 소켓 시스템 콜을 획기적으로 줄이고 어뷰징에도 면역이 됩니다. 클라 측 뚝뚝 끊김은 Dead Reckoning + 보간으로 처리하도록 유도합니다."

**Q. 직렬화 부하는 어떻게 줄이나?**
> "유저별 시야가 달라 개별 직렬화하면 O(N²)입니다. 시야 계산을 격자 기반으로 바꾸면, 격자별 이동 리스트를 한 번만 직렬화해 그 칸 전원이 같은 SendBuffer를 공유하므로 직렬화 비용이 격자 수에 비례(O(N))합니다. 경계선 문제는 주변 9칸 패킷을 모아 보내 해결합니다."

---

## 부록: 실제 코드와의 매핑

이 문서의 의사코드(`GameRoom`/`Sector`)는 일반론이며, 본 프로젝트의 실제 구조와는 다음과 같이 대응한다.

| 문서 용어 | 실제 프로젝트 |
|---|---|
| `GameRoom` | `World` + `GameScene` (씬 단위 JobQueue 스레드) |
| `Sector` / Grid | `Zone` (`Zone.h`, 3x3 = `GetAdjacentZones()`) |
| 시야 교체 연산 | `GameScene::HandleZoneChange` + `World::CalculateMoveResult` (enter/leave/keep) |
| `_myViewport` | (미구현) 현재는 Zone 멤버십이 암묵적 view 역할 |
| 인원 제한 / Hysteresis | (미구현) 현재 enter/leave 반경 동일(3x3) |
| 틱 배칭 | `GameScene::PushMoveJob` → `Update()`에서 일괄 처리 (부분 구현) |
| 패킷 번들링 | `SUpdateScene` (spawns/despawns/moves repeated 필드 = 이미 번들) |
| Grid 단위 버퍼 공유 | `BroadcastToZone`이 존당 동일 `SendBuffer` 공유 (이미 구현) |

> **결론:** 본 프로젝트는 9장(Grid 단위 직렬화 공유)·8장(번들링)은 이미 구현된 상태이고, **4장(인원 제한+우선순위)·5장(Hysteresis 마진)·1장(시야 갱신 틱 분리)** 이 다음 개선 후보다.
