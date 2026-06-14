# API / 패킷 명세 (Protocol Spec)

서버는 3개 채널을 쓴다. 인증만 HTTP(REST), 실시간은 TCP 소켓(Protobuf).

| 채널 | 서버 | 프로토콜 | 용도 |
|---|---|---|---|
| 웹 REST | LoginWebServer | HTTP / JSON | 회원가입·로그인·서버목록·토큰 발급 |
| 게이트 소켓 | GateServer | TCP / Protobuf | 대기열 입장 제어 → 게임 서버 정보 발급 |
| 게임 소켓 | GameServer | TCP / Protobuf | 인게임 실시간 동기화 |

> 소켓 프레이밍: `[헤더(size, id)] + [Protobuf payload]`. 부분 수신은 RecvBuffer에 누적 후 완전한 패킷 단위로 파싱.

---

## 1. 웹 REST API (LoginWebServer · `Route("api/auth")`)

### POST /api/auth/create — 회원가입
중복 아이디 검사 후 비밀번호를 BCrypt 해싱해 저장.

```jsonc
// Request
{ "accountName": "player1", "password": "..." }
// Response
{ "success": true }     // 아이디 중복/검증 실패 시 success=false
```

### POST /api/auth/login — 로그인
MySQL 조회 + `BCrypt.Verify` → 성공 시 Guid 토큰을 Redis(`qsession:{token}`, TTL)에 저장하고 게이트 정보·서버목록 반환.

```jsonc
// Request
{ "accountName": "player1", "password": "..." }
// Response
{
  "success": true,
  "accountId": 1001,
  "queueToken": "3f9c...",          // 게이트 접속용 1회성 토큰 (Redis 저장)
  "gateIp": "127.0.0.1", "gatePort": 6666,
  "serverList": [
    { "id":1, "serverName":"Server1", "ipAddress":"127.0.0.1", "port":7777,
      "currentCount":1200, "maxCount":3000 }
  ]
}
```

---

## 2. 전체 접속 흐름

```
[Client] --HTTP--> LoginWebServer : create / login → queueToken + gateIp/port + serverList
[Client] --TCP--->  GateServer    : C_Q_ENTER(queueToken, serverId)
                                    GateServer가 Redis로 토큰 검증 → 대기열 → 정원 게이트
        <--------                  S_Q_STATUS(순번)  ... → S_Q_ADMITTED(authToken, gameIp/port)
[Client] --TCP--->  GameServer    : C_AUTH_TOKEN(authToken)
                                    GameServer가 Redis로 토큰 검증(1회용, 즉시 삭제) → 입장
        <--------                  S_PLAYER_LIST → (C_ENTER_GAME) → S_ENTER_GAME
```

### 게이트 패킷 (GateServer)
| 패킷 | 방향 | 주요 필드 |
|---|---|---|
| `C_Q_ENTER` | C→Gate | queue_token, server_id |
| `S_Q_ENTER` | Gate→C | success, reason |
| `S_Q_STATUS` | Gate→C | position(대기 순번), total, eta_sec |
| `S_Q_ADMITTED` | Gate→C | auth_token(게임 접속용), game_ip, game_port |

---

## 3. 게임 패킷 (GameServer · `Protocol.proto`)

### MsgId 목록
| Id | 패킷 | 방향 | 용도 |
|---|---|---|---|
| 1 | C_AUTH_TOKEN | C→S | 토큰 인증·입장 |
| 2 | S_PLAYER_LIST | S→C | 캐릭터 목록 |
| 3/4 | C/S_CREATE_PLAYER | ↔ | 캐릭터 생성(이름 중복 검사) |
| 6/7 | C/S_ENTER_GAME | ↔ | 인게임 입장 |
| 8 | S_READY_TO_ENTER | S→C | 주변 스폰 전송 완료 |
| 9/10 | C/S_LEAVE_GAME | ↔ | 퇴장 |
| 11 | **S_UPDATE_SCENE** | S→C | AOI 번들: spawns/despawns/moves |
| 12 | **C_MOVE** | C→S | 이동(위치+속도+상태) |
| 25 | S_MOVE_CORRECTION | S→C | 서버 권위 좌표 롤백 |
| 13/14 | C/S_ATTACK | ↔ | 공격 |
| 18 | S_CHANGE_HP | S→C | HP/데미지 |
| 15/16/17 | S_DIE / C_REVIVE / S_REVIVE | ↔ | 사망·부활 |
| 19/20 | S_CHANGE_EXP / S_CHANGE_LEVEL | S→C | 경험치·레벨업 |
| 21/22 | C/S_CHAT | ↔ | 채팅(근처/월드) |
| 23/24 | C/S_TIME_SYNC | ↔ | 클럭 동기화(이동 보간용) |

### 핵심 메시지 구조
```proto
message PosInfo  { uint64 object_id; CreatureState state; Vector3 pos; float yaw; Vector3 velocity; }

message CMove        { PosInfo pos_info; uint64 send_server_time_ms; }   // 이동 요청(속도 포함)
message SUpdateScene { repeated ObjectInfo spawns; repeated uint64 despawns; repeated PosInfo moves; } // AOI 번들
message SMoveCorrection { Vector3 pos; float yaw; }                      // 거부 시 롤백

message CAttack { float yaw; int32 combo_index; Vector3 pos; }
message SChangeHp { uint64 object_id; int32 hp; int32 damage; }
message CChat { string chat; ChatType chat_type; }   // CHAT_NEAR(주변 존) / CHAT_WORLD(전체)
```

### 대표 흐름
- **이동**: `C_MOVE` → 서버 권위 검증(walkability·속도) → 50ms 틱에 변화분만 `S_UPDATE_SCENE`(존 번들) 브로드캐스트. 거부 시 `S_MOVE_CORRECTION`.
- **전투**: `C_ATTACK` → 서버 히트 판정(범위·각도) → `S_ATTACK`/`S_CHANGE_HP` → 사망 시 `S_DIE`(+`S_CHANGE_EXP/LEVEL`).
- **채팅**: `C_CHAT` → `CHAT_NEAR`는 주변 존, `CHAT_WORLD`는 전체로 `S_CHAT` 브로드캐스트(스팸 쿨다운).

> 서버는 모든 게임 로직을 **권위적으로 판정**한다. 클라가 보낸 위치·데미지·결과를 그대로 신뢰하지 않는다.
