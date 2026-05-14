#include "pch.h"
#include "IocpEvent.h"

IocpEvent::IocpEvent(IocpEventType type) : _eventType(type)
{
    Init();
}

void IocpEvent::Init()
{
    ::ZeroMemory(static_cast<OVERLAPPED*>(this), sizeof(OVERLAPPED));
}
