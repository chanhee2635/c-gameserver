#pragma once

using SessionFactory = std::function<SessionRef()>;

class Service : public std::enable_shared_from_this<Service>
{
public:
    Service(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount);
    virtual ~Service() = default;

    virtual bool Start() = 0;

    void IocpDispatch(uint32 timeoutMs = 10);

    SessionRef CreateSession();
    void       AddSession(SessionRef session);
    void       ReleaseSession(SessionRef session);

    int32           GetCurrentSessionCount() const { return _sessionCount; }
    int32           GetMaxSessionCount()     const { return _maxSessionCount; }
    NetAddress      GetAddress()             const { return _address; }
    IocpCoreRef&    GetIocpCore() { return _iocpCore; }

    template<typename Fn>
    void ForEachSession(Fn&& fn)
    {
        std::lock_guard guard(_lock);
        for (auto& session : _sessions)
            fn(session);
    }

protected:
    NetAddress          _address;
    SessionFactory      _sessionFactory;
    IocpCoreRef         _iocpCore;

private:
    std::mutex          _lock;
    HashSet<SessionRef> _sessions;
    std::atomic<int32>  _sessionCount = 0;
    int32               _maxSessionCount = 0;
};

class ServerService : public Service
{
public:
    ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount);
    virtual bool Start() override;

private:
    ListenerRef _listener;
};

class ClientService : public Service
{
public:
    ClientService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount);
    virtual bool Start() override;

private:
    void Connect();
};