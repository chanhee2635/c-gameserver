#pragma once
#include <memory>

class DummySession;
class DummyGateSession;
class DummySimulator;

using DummySessionRef     = std::shared_ptr<DummySession>;
using DummySessionWeakRef = std::weak_ptr<DummySession>;
using DummyGateSessionRef = std::shared_ptr<DummyGateSession>;  
using DummySimulatorRef   = std::shared_ptr<DummySimulator>;