#include "pch.h"
#include "DummyGlobal.h"
#include "DummySimulator.h"
#include "DummyNet.h"
#include "IocpCore.h"

ClientServiceRef                DummyGlobal::_service = nullptr;
std::unique_ptr<DummySimulator> DummyGlobal::_simulator = nullptr;
std::unique_ptr<DummyNet>       DummyGlobal::_net = nullptr;

DummySimulator* GDummySimulator = nullptr;
DummyNet* GDummyNet = nullptr;

void DummyGlobal::Init()
{
    CoreGlobal::Init();

    _simulator = std::make_unique<DummySimulator>();
    GDummySimulator = _simulator.get();
    GDummySimulator->Start();

    _service = MakeShared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        MakeShared<IocpCore>(),
        []() -> SessionRef { return nullptr; },   // 직접 연결하므로 미사용
        0);
    ASSERT_CRASH(_service->Start());

    _net = std::make_unique<DummyNet>();
    _net->service = _service;
    _net->gameAddr = NetAddress(L"127.0.0.1", 7777);
    _net->gateAddr = NetAddress(L"127.0.0.1", 6666);
    GDummyNet = _net.get();

    LOG_INFO(L"=== [DummyGlobal Ready] ===");
}

void DummyGlobal::Clear()
{
    if (GDummySimulator) GDummySimulator->Stop();

    GDummyNet = nullptr;
    _net = nullptr;
    GDummySimulator = nullptr;
    _simulator = nullptr;
    _service = nullptr;

    CoreGlobal::Clear();
}