#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"

class Listener : public IocpObject
{
private:
    class AcceptEvent : public IocpEvent
    {
    public:
        AcceptEvent() : IocpEvent(IocpEventType::Accept) {}

        SessionRef GetSession() const { return _session; }
        void       SetSession(SessionRef s) { _session = s; }
        char*      GetAddressBuffer() { return _addressBuffer; }

    private:
        SessionRef _session = nullptr;
        alignas(Config::Network::ADDR_BUFFER_ALIGNMENT)
            char _addressBuffer[Config::Network::ADDR_BUFFER_SIZE] = {};
    };

public:
    Listener() = default;
    ~Listener();

    bool StartAccept(ServerServiceRef service);
    void Close();

    virtual HANDLE GetHandle() const override { return reinterpret_cast<HANDLE>(_listenSocket); }
    virtual void   Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
    void RegisterAccept(AcceptEvent* acceptEvent);
    void ProcessAccept(AcceptEvent* acceptEvent);

    SOCKET               _listenSocket = INVALID_SOCKET;
    Vector<AcceptEvent*> _acceptEvents;
    ServerServiceRef     _service;
};
