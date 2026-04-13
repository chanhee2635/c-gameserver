#include "pch.h"
#include "DummyUser.h"
#include "GameSession.h"
#include "ChatSession.h"
#include "Service.h"
#include "Chat.pb.h"
#include "ServerPacketHandler.h"
#include "NavigationManager.h"


void DummyUser::InitInfo(int32 dummyId)
{
	_name = "Dummy_" + std::to_string(dummyId);
	_templateId = (rand() % 2) + 1;
}

void DummyUser::ConnectToGame()
{
	_gameSession = static_pointer_cast<GameSession>(GGameService->CreateSession());
	if (_gameSession == nullptr) return;

	_gameSession->SetOwner(shared_from_this());
	_gameSession->Connect();
}

void DummyUser::ConnectToChat()
{
	ChatStatus expected = ChatStatus::None;
	if (_chatStatus.compare_exchange_strong(expected, ChatStatus::Connecting) == false)
		return;

	_chatSession = static_pointer_cast<ChatSession>(GChatService->CreateSession());
	if (_chatSession == nullptr)
	{
		_chatStatus.store(ChatStatus::None);
		return;
	}

	_chatSession->SetOwner(shared_from_this());
	_chatSession->Connect();
}

void DummyUser::Update(int64 now, float deltaTime)
{
	if (_gameSession == nullptr || _gameSession->IsConnected() == false)
		return;
	if (_chatSession == nullptr || _chatStatus.load() != ChatStatus::Connected)
		return;

	if (now >= _nextChatTick)
	{
		_nextChatTick = now + (10000 + (rand() % 20000));
		SendRandomChat();
	}

	if (now > _nextMoveTick)
	{
		_nextMoveTick = now + (3000 + (rand() % 2000));

		int32 angle = rand() % 360;
		_moveDir = DirectionTable::Directions[angle];
		float radian = atan2f(_moveDir.x, _moveDir.z);
		_currentYaw = radian * (180.0f / 3.141592f);
	}

	if (_gameSession->GetState() == Protocol::CreatureState::IDLE && now < _nextMoveTick)
		return;
	
	ContinuousMove(deltaTime);
	
	if (now >= _nextMovePacketTick)
	{
		_nextMovePacketTick = now + MOVE_PACKET_INTERVAL;
		if (_gameSession->GetState() == Protocol::CreatureState::MOVING)
			SendMovePacket();
	}
}

void DummyUser::Disconnect()
{
	if (_gameSession)
	{
		_gameSession->Disconnect(L"DummyClient Logout");
		_gameSession->SetOwner(nullptr);
		_gameSession = nullptr;
	}
	if (_chatSession)
	{
		_chatSession->Disconnect(L"DummyClient Logout");
		_chatSession->SetOwner(nullptr);
		_chatSession = nullptr;
	}
}

void DummyUser::Clear()
{
	Disconnect();

	_gameSession = nullptr;
	_chatSession = nullptr;

	_chatStatus.store(ChatStatus::None);

}

void DummyUser::ContinuousMove(float deltaTime)
{
	Vector3 nextPos = _pos;
	nextPos.x += _moveDir.x * MOVE_SPEED * deltaTime;
	nextPos.z += _moveDir.z * MOVE_SPEED * deltaTime;

	if (GNavigationManager->CanMoveTo(nextPos))
	{
		_pos = nextPos;
		_gameSession->SetState(Protocol::CreatureState::MOVING);
	}
	else
	{
		_nextMoveTick = ::GetTickCount64() + 2000;
		_gameSession->SetState(Protocol::CreatureState::IDLE);
		return;
	}

	_gameSession->SetPosX(_pos.x);
	_gameSession->SetPosZ(_pos.z);
	_gameSession->SetYaw(_currentYaw);
}

void DummyUser::SendToGame(SendBufferRef sendBuffer)
{
	_gameSession->Send(sendBuffer);
}

void DummyUser::SendRandomChat()
{
	if (_chatSession == nullptr || _chatStatus.load() != ChatStatus::Connected)
		return;

	string msg = "Hi! I'm Dummy" + _gameSession->GetName();

	Protocol::C_Chat packet;
	packet.set_msg(msg);
	packet.set_toserver(false);

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	_chatSession->Send(sendBuffer);
}

void DummyUser::SendMovePacket()
{
	if (_gameSession == nullptr || _gameSession->IsConnected() == false)
		return;

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