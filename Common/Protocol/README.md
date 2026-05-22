# Gen.bat — 패킷 자동 생성 도구

`Protocol.proto` 하나를 수정하면 C++, C# 코드를 자동 생성합니다.

---

## 실행 방법

```
Common\Protocol\Gen.bat
```

---

## 생성 순서

### Step 1 — `GenProtocol.ps1` (protobuf 컴파일)

`Protocol.proto` → C++ 및 C# 파일 생성

| 출력 파일 | 위치 | 조건 |
|-----------|------|------|
| `Protocol.pb.h` / `Protocol.pb.cc` | `Server/Protocol/`, `DummyClient/Protocol/` | proto보다 오래됐을 때만 재생성 |
| `Protocol.cs` | `Client/Assets/Scripts/Packet/` | proto보다 오래됐을 때만 재생성 |

### Step 2 — `GenPackets.ps1` (핸들러 생성)

`Protocol.proto`의 MsgId enum을 파싱하여 핸들러 코드 생성

#### C++ (Server / DummyClient)

| 출력 파일 | 위치 | 덮어쓰기 |
|-----------|------|---------|
| `ClientPacketHandler.h` | `Server/Protocol/` | 항상 |
| `ClientPacketHandler_Generated.cpp` | `Server/Protocol/` | 항상 |
| `ClientPacketHandler.cpp` | `Server/` | **최초 1회만** |

#### Unity C#

| 출력 파일 | 위치 | 덮어쓰기 |
|-----------|------|---------|
| `PacketManager_Generated.cs` | `Client/Assets/Scripts/Network/` | 항상 |
| `PacketHandler.cs` | `Client/Assets/Scripts/Packet/` | **최초 1회만** |

---

## 파일별 역할

### 자동 생성 — 직접 수정 금지

| 파일 | 내용 |
|------|------|
| `Protocol.pb.h` / `Protocol.pb.cc` | protobuf 직렬화/역직렬화 C++ 코드 |
| `Protocol.cs` | protobuf 직렬화/역직렬화 C# 코드 |
| `ClientPacketHandler.h` | `C_` 패킷 핸들러 선언 + `MakeSendBuffer` 템플릿 |
| `ClientPacketHandler_Generated.cpp` | `Handle()` — type으로 분기하는 switch 문 |
| `PacketManager_Generated.cs` | `PacketManager.Register()` — `S_` 패킷 핸들러 등록, `PacketIdMapper` — `C_` 패킷 ID 맵 |

### 수동 작성 — Gen.bat이 덮어쓰지 않음

| 파일 | 내용 |
|------|------|
| `ClientPacketHandler.cpp` | C++ 핸들러 구현 (서버 로직) |
| `PacketHandler.cs` | Unity 핸들러 구현 (클라이언트 로직) |

---

## 네이밍 규칙

proto enum 값 → 클래스명 변환:

```
S_PLAYER_LIST  →  SPlayerList
C_AUTH_TOKEN   →  CAuthToken
S_UPDATE_SCENE →  SUpdateScene
```

- 언더스코어로 분리 후 각 단어 첫 글자 대문자

---

## C++ 패킷 송신

```cpp
// MakeSendBuffer<PacketType>(msg) — ClientPacketHandler.h에 정의됨
session->Send(MakeSendBuffer<Protocol::MsgId::S_PLAYER_LIST>(packet));
```

## C# 패킷 송신

```csharp
// PacketIdMapper가 타입으로 ID를 자동 조회
Managers.Network.Send(new Protocol.CAuthToken { AuthToken = token });
```

---

## 스킵 문제 해결 (Up to date, skipping)

생성된 파일이 proto보다 최신 타임스탬프인 경우 자동 스킵됩니다.  
아래 파일들을 삭제 후 Gen.bat 재실행:

**C++ 재생성 필요 시:**
```
Server/Protocol/Protocol.pb.h
Server/Protocol/Protocol.pb.cc
```

**C# 재생성 필요 시:**
```
Client/Assets/Scripts/Packet/Protocol.cs
```

---

## 흐름 요약

```
Protocol.proto
    │
    ├─ GenProtocol.ps1 ──► Protocol.pb.h / .pb.cc  (Server, DummyClient)
    │                  ──► Protocol.cs              (Unity)
    │
    └─ GenPackets.ps1  ──► ClientPacketHandler.h            (Server - 항상)
                       ──► ClientPacketHandler_Generated.cpp (Server - 항상)
                       ──► ClientPacketHandler.cpp           (Server - 최초 1회)
                       ──► PacketManager_Generated.cs        (Unity - 항상)
                       ──► PacketHandler.cs                  (Unity - 최초 1회)
```
