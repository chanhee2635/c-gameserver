#pragma once

enum class IocpEventType : uint8 { Accept, Connect, Disconnect, Recv, Send };

class IocpEvent : public OVERLAPPED
{
public:
    IocpEvent(IocpEventType type);

    void          Init();

    IocpEventType GetType()  const { return _eventType; }
    IocpObjectRef GetOwner() const { return _owner; }
    void          SetOwner(IocpObjectRef owner) { _owner = owner; }

private:
    IocpEventType _eventType;
    IocpObjectRef _owner;
};
