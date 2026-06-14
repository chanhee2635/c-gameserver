# DB 설계 (Database Schema)

MySQL 하나에 **인증(웹)** 과 **게임(인게임)** 데이터를 담고, 접근 경로를 분리한다.

| 영역 | 접근 주체 | 방식 | 테이블 |
|---|---|---|---|
| 계정 | LoginWebServer (.NET8) | EF Core (Code-First Migration) | `Accounts` |
| 캐릭터 | GameServer (C++) | ODBC + 커넥션 풀 | `Players`, `PlayerDetails` |

> 세션/토큰·서버 레지스트리 같은 휘발성 데이터는 DB가 아니라 **Redis**에 둔다(영속화 대상 아님).

---

## ERD

```
 Accounts (계정)                Players (캐릭터)              PlayerDetails (캐릭터 상태)
 ┌───────────────┐  1     N  ┌───────────────┐  1     1  ┌──────────────────┐
 │ AccountId  PK │───────────│ player_id  PK │───────────│ player_id  PK/FK │
 │ AccountName ●U│           │ account_id FK │           │ hp, mp, exp      │
 │ PasswordHash  │           │ name          │           │ pos_x,pos_y,pos_z│
 │ CreatedAt     │           │ level         │           │ rot_y            │
 └───────────────┘           │ template_id   │           └──────────────────┘
        ●U = Unique Index    └───────────────┘
```

- **1 계정 : N 캐릭터** (`Players.account_id` → `Accounts.AccountId`)
- **1 캐릭터 : 1 상세** (`PlayerDetails.player_id` → `Players.player_id`)
- 기본 정보(`Players`)와 자주 변하는 상태(`PlayerDetails`)를 분리 → 목록 조회 시 가벼운 `Players`만 읽는다.

---

## 테이블

### Accounts — 계정 (LoginWebServer / EF Core)

| 컬럼 | 타입 | 키/제약 | 설명 |
|---|---|---|---|
| AccountId | int | **PK**, auto-increment | 계정 식별자 |
| AccountName | string | **Unique Index** (`IX_Accounts_AccountName`) | 로그인 ID (중복 불가) |
| PasswordHash | string | | **BCrypt 해시** (평문 미저장) |
| CreatedAt | datetime | | 생성 시각(UTC) |

### Players — 캐릭터 기본 (GameServer / ODBC)

| 컬럼 | 타입 | 키/제약 | 설명 |
|---|---|---|---|
| player_id | bigint | **PK**, auto-increment | 캐릭터 식별자 |
| account_id | bigint | **FK** → Accounts.AccountId | 소유 계정 |
| name | varchar | | 캐릭터 이름 |
| level | int | | 레벨 |
| template_id | int | | 직업/외형 프리팹 id |

### PlayerDetails — 캐릭터 상태 (GameServer / ODBC)

| 컬럼 | 타입 | 키/제약 | 설명 |
|---|---|---|---|
| player_id | bigint | **PK / FK** → Players.player_id | 캐릭터 |
| hp, mp | int | | 체력/마나 |
| exp | bigint | | 경험치 |
| pos_x, pos_y, pos_z | float | | 마지막 위치(로그아웃 좌표 복원) |
| rot_y | float | | 마지막 방향(yaw) |

---

## 트랜잭션 — 캐릭터 생성

`DBManager::CreatePlayer` 는 두 테이블 INSERT를 **한 트랜잭션**으로 묶는다 (`autocommit=OFF` → `Commit`/`Rollback`).

```
1) BEGIN  (SetAutoCommit OFF)
2) INSERT INTO Players (account_id, name, level, template_id) VALUES (?, ?, 1, ?)
3) SELECT LAST_INSERT_ID()                 -- 같은 커넥션에서 새 player_id 획득
4) INSERT INTO PlayerDetails (player_id, pos_x, pos_y, pos_z, rot_y) VALUES (...)
5) COMMIT  (중간 실패 시 ROLLBACK)
```

> 왜 트랜잭션인가: `Players`만 들어가고 `PlayerDetails`가 빠지면 **상태 없는 유령 캐릭터**가 생긴다. 둘은 원자적으로 함께 커밋되어야 정합성이 유지된다.

---

## 영속화 — Write-Back

이동마다 DB를 건드리지 않는다. 메모리에 누적하다가 **dirty 시점(레벨업·로그아웃 등)에만 비동기 저장**한다.

- `SavePlayerInfo` → `UPDATE PlayerDetails SET hp=?, mp=?, exp=?, pos_x=?, pos_y=?, pos_z=?, rot_y=? WHERE player_id=?`
- 레벨 변경 시 추가로 `UPDATE Players SET level=? WHERE player_id=?`
- 저장은 ODBC 커넥션 풀에서 워커가 비동기 처리 → 게임 틱을 막지 않음. (부하 테스트 실측 `DB Queries ≈ 0/s`, `Failed 0`)

---

## 데이터 흐름 요약

| 요청 | 읽기 | 쓰기 |
|---|---|---|
| 캐릭터 목록(`C_AUTH_TOKEN`→`S_PLAYER_LIST`) | `Players WHERE account_id=?` | — |
| 캐릭터 생성(`C_CREATE_PLAYER`) | 이름 중복 검사 | `Players` + `PlayerDetails` (트랜잭션) |
| 입장(`C_ENTER_GAME`) | `Players`+`PlayerDetails` 조인 로드 | — |
| 인게임 진행 | (메모리) | dirty 시 Write-Back UPDATE |
