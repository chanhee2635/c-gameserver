#include "pch.h"
#include "SessionManager.h"

void SessionManager::Register(uint64 accountId, GameSessionRef session)
{
    WRITE_LOCK;
    auto it = _sessions.find(accountId);
    if (it != _sessions.end())
    {
        if (auto old = it->second.lock())
        {
            LOG_WARN(L"중복 로그인 기존 세션 해제 accountId=" + std::to_wstring(accountId));
            old->Disconnect();
        }
    }
    _sessions[accountId] = session;
}

void SessionManager::Unregister(uint64 accountId)
{
    WRITE_LOCK;
    _sessions.erase(accountId);
}

GameSessionRef SessionManager::Find(uint64 accountId)
{
    READ_LOCK;
    auto it = _sessions.find(accountId);
    if (it == _sessions.end()) return nullptr;
    return it->second.lock();
}
