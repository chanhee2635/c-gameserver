#pragma once
#include "Service.h"

class DummySimulator;
struct DummyNet;

extern DummySimulator* GDummySimulator;
extern DummyNet* GDummyNet;

class DummyGlobal
{
public:
    static void Init();
    static void Clear();
    static const ClientServiceRef& GetService() { return _service; }

private:
    static ClientServiceRef                _service;
    static std::unique_ptr<DummySimulator> _simulator;
    static std::unique_ptr<DummyNet>       _net;
};