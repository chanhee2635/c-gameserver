#pragma once
#include "Session.h" 
#include "Service.h"
#include "IocpCore.h"
#include "DummyTypes.h"
#include "DummySession.h"
#include "DummyGateSession.h"

struct DummyNet
{
    ClientServiceRef service;
    NetAddress       gameAddr;
    NetAddress       gateAddr;

    void ConnectGate(DummyGateSessionRef s) { Connect(s, gateAddr); }
    void ConnectGame(DummySessionRef s) { Connect(s, gameAddr); }

    void Connect(SessionRef s, const NetAddress& addr)
    {
        s->SetService(service);
        if (service->GetIocpCore()->Register(s))
            s->RegisterConnect(addr);
    }
};

extern DummyNet* GDummyNet;