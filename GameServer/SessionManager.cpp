#include "pch.h"
#include "SessionManager.h"

void SessionManager::Register(uint64 accountId, GameSessionRef session)
{
    GameSessionRef old;
    {
        WRITE_LOCK;
        auto it = _sessions.find(accountId);
        if (it != _sessions.end())
            old = it->second.lock();
        _sessions[accountId] = session;   
    }
    if (old)
    {
        LOG_WARN(L"Duplicate login, releasing existing session accountId=" + std::to_wstring(accountId));
        old->Disconnect();                
    }
}

void SessionManager::Unregister(uint64 accountId, GameSession* self)
{
    WRITE_LOCK;
    auto it = _sessions.find(accountId);
    if (it == _sessions.end()) return;
    auto cur = it->second.lock();
    if (!cur || cur.get() == self)
        _sessions.erase(it);
}

GameSessionRef SessionManager::Find(uint64 accountId)
{
    READ_LOCK;
    auto it = _sessions.find(accountId);
    if (it == _sessions.end()) return nullptr;
    return it->second.lock();
}
