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
	_gameSession->SetService(GGameService);
	_gameSession->SetOwner(shared_from_this());
	_gameSession->Connect();
}

void DummyUser::ConnectToChat()
{
	_chatSession = static_pointer_cast<ChatSession>(GChatService->CreateSession());
	_chatSession->SetService(GChatService);
	_chatSession->SetOwner(shared_from_this());
	_chatSession->Connect();
}

void DummyUser::Update(uint64 now)
{
	if (isConnected == false) return;

	if (now > _nextChatTick)
	{
		_nextChatTick = now + 2000 + (rand() % 3000);

		if (rand() % 100 < 20)
		{
			SendRandomChat();
		}
	}

	if (now > _nextMoveTick)
	{
		_nextMoveTick = now + 1000;
		SendRandomMove();
	}
}

void DummyUser::SendRandomChat()
{
	string msg = u8"Hello, I'm " + _gameSession->GetName();

	Protocol::C_Chat packet;
	packet.set_msg(msg);
	packet.set_toserver(rand() % 2 == 0);

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	_chatSession->Send(sendBuffer);
}

void DummyUser::SendRandomMove()
{
	_gameSession->SetPosX((rand() % 3 - 1));
	_gameSession->SetPosZ((rand() % 3 - 1));
	_gameSession->SetState(Protocol::CreatureState::MOVING);

	Protocol::C_Move packet;
	auto* posInfo = packet.mutable_pos();
	posInfo->CopyFrom(_gameSession->GetPosInfo());

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	_gameSession->Send(sendBuffer);
}

uint64 DummyUser::GetObjectId()
{
	return _gameSession->GetObjectId();
}
