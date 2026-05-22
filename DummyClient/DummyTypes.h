#pragma once
#include <memory>

class DummySession;
class DummySimulator;

using DummySessionRef     = std::shared_ptr<DummySession>;
using DummySessionWeakRef = std::weak_ptr<DummySession>;
using DummySimulatorRef   = std::shared_ptr<DummySimulator>;