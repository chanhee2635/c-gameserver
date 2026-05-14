#pragma once
#include "GameSession.h"

class SessionManager
{
public:
    void Register(uint64 accountId, GameSessionRef session);
    void Unregister(uint64 accountId);
    GameSessionRef Find(uint64 accountId);

private:
    USE_LOCK;
    HashMap<uint64, GameSessionWeakRef> _sessions;
};