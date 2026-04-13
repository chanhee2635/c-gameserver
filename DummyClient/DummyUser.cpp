#include "pch.h"
#include "DummyUser.h"
#include "GameSession.h"
#include "ChatSession.h"
#include "Service.h"
#include "Chat.pb.h"
#include "ServerPacketHandler.h"

void DummyUser::ConnectToGame()
{
	_gameSession = static_pointer_cast<GameSession>(GGameService->CreateSession());
	if (_gameSession == nullptr) return;

	_gameSession->SetOwner(shared_from_this());
	_gameSession->Connect();
}

void DummyUser::ConnectToChat()
{
	_chatSession = static_pointer_cast<ChatSession>(GChatService->CreateSession());
	if (_chatSession == nullptr) return;

	_chatSession->SetOwner(shared_from_this());
	_chatSession->Connect();
}

void DummyUser::Update(int64 now)
{
	if (isConnected == false) return;

	if (now > _nextChatTick)
	{
		_nextChatTick = now + 2000 + (rand() % 3000);
		if (rand() % 100 < 20)
			SendRandomChat();
	}

	int64 elapsed = (_lastMoveSendTick > 0) ? (now - _lastMoveSendTick) : (int64)MOVE_SEND_INTERVAL_MS;
	float deltaTime = (float)elapsed / 1000.f;

	if (now > _nextMoveTick)
	{
		_nextMoveTick = now + (rand() % 2000 + 3000);

		float angle = (rand() % 360) * (3.141592f / 180.0f);
		_moveDir = { cosf(angle), 0.f, sinf(angle) };
		float yaw = atan2f(_moveDir.x, _moveDir.z) * (180.0f / 3.141592f);
		if (yaw < 0) yaw += 360.0f;

		if (_gameSession)
		{
			_gameSession->SetState(Protocol::CreatureState::MOVING);
			_gameSession->SetYaw(yaw);
		}

		_lastMoveSendTick = now;
		ContinuousMove(0.f);
		return;
	}

	if (elapsed < (int64)MOVE_SEND_INTERVAL_MS) return;
	ContinuousMove(deltaTime);
}

void DummyUser::Disconnect()
{
	if (_gameSession)
		_gameSession->Disconnect(L"DummyClient Remove");
	if (_chatSession)
		_chatSession->Disconnect(L"DummyClient Remove");
}

void DummyUser::ContinuousMove(float deltaTime)
{
	if (_gameSession == nullptr) return;

	int64 now = ::GetTickCount64();
	_lastMoveSendTick = now;

	float curX = _gameSession->GetPosX();
	float curZ = _gameSession->GetPosZ();
	float newX = curX + _moveDir.x * MOVE_SPEED * deltaTime;
	float newZ = curZ + _moveDir.z * MOVE_SPEED * deltaTime;

	constexpr float MAP_MIN = 5.0f;
	constexpr float MAP_MAX = 495.0f;

	bool dirChanged = false;
	if (newX < MAP_MIN || newX > MAP_MAX)
	{
		_moveDir.x = -_moveDir.x;
		newX = std::clamp(newX, MAP_MIN, MAP_MAX);
		dirChanged = true;
	}
	if (newZ < MAP_MIN || newZ > MAP_MAX)
	{
		_moveDir.z = -_moveDir.z;
		newZ = std::clamp(newZ, MAP_MIN, MAP_MAX);
		dirChanged = true;
	}

	if (dirChanged)
	{
		float yaw = atan2f(_moveDir.x, _moveDir.z) * (180.0f / 3.141592f);
		if (yaw < 0) yaw += 360.0f;
		_gameSession->SetYaw(yaw);
	}

	_gameSession->SetPosX(newX - curX);
	_gameSession->SetPosZ(newZ - curZ);

	SendMovePacket();
}

void DummyUser::SendRandomChat()
{
	if (_chatSession == nullptr) return;
	if (_gameSession == nullptr) return;

	string msg = u8"Hello, I'm " + _gameSession->GetName();

	Protocol::C_Chat packet;
	packet.set_msg(msg);
	packet.set_toserver(rand() % 2 == 0);

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	_chatSession->Send(sendBuffer);
}

void DummyUser::SendMovePacket()
{
	if (_gameSession == nullptr) return;

	Protocol::C_Move packet;
	auto* posInfo = packet.mutable_pos_info();
	posInfo->CopyFrom(_gameSession->GetPosInfo());

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	_gameSession->Send(sendBuffer);
}

uint64 DummyUser::GetObjectId()
{
	if (_gameSession == nullptr) return 0;
	return _gameSession->GetObjectId();
}