#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"
#include "IocpCore.h"

Service::Service(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
    : _address(address)
    , _iocpCore(core)
    , _sessionFactory(factory)
    , _maxSessionCount(maxSessionCount)
{}

void Service::IocpDispatch(uint32 timeoutMs)
{
    _iocpCore->Dispatch(timeoutMs);
}

SessionRef Service::CreateSession()
{
    SessionRef session = _sessionFactory();
    session->SetService(shared_from_this());
    if (!_iocpCore->Register(session))
    {
        LOG_ERROR(L"Service: IOCP register failed");
        return nullptr;
    }
    return session;
}

void Service::AddSession(SessionRef session)
{
    LockGuard guard(_lock);
    _sessionCount++;
    _sessions.insert(session);
}

void Service::ReleaseSession(SessionRef session)
{
    LockGuard guard(_lock);
    _sessionCount--;
    _sessions.erase(session);
}

/*--------------
  ServerService
--------------*/
ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
    : Service(address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
    _listener = MakeShared<Listener>();
    ASSERT_CRASH(_listener != nullptr);
    return _listener->StartAccept(static_pointer_cast<ServerService>(shared_from_this()));
}

ClientService::ClientService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount) 
    : Service(address, core, factory, maxSessionCount)
{}

bool ClientService::Start()
{
    for (int32 i = 0; i < GetMaxSessionCount(); i++)
        Connect();
    return true;
}

void ClientService::Connect()
{
    SessionRef session = CreateSession();
    ASSERT_CRASH(session != nullptr);
    session->RegisterConnect(GetAddress());
}
