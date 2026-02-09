#include "pch.h"
#include "Room.h"
#include "GameObject.h"
#include "Player.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"

shared_ptr<Room> GRoom = make_shared<Room>();

void Room::Update()
{
	for (auto var : _players)
	{
		//var.second->Update();
	}

	GRoom->DoTimer(200, &Room::Update);
}


void Room::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& p : _players)
	{
		p.second->Send(sendBuffer);
	}
}
