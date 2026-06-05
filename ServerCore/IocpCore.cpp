#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"
#include "ServerStats.h"

IocpCore::IocpCore()
{
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
    if (_iocpHandle != INVALID_HANDLE_VALUE)
        ::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
    return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, reinterpret_cast<ULONG_PTR>(iocpObject.get()), 0) != NULL;
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
    DWORD       numOfBytes = 0;
    IocpObject* iocpObject = nullptr;
    OVERLAPPED* overlapped = nullptr;

    if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT reinterpret_cast<PULONG_PTR>(&iocpObject), OUT & overlapped, timeoutMs))
    {
        if (iocpObject && overlapped)
            ProcessEvent(iocpObject, static_cast<IocpEvent*>(overlapped), numOfBytes);
    }
    else
    {
        const int32 errCode = ::WSAGetLastError();
        if (overlapped)
            ProcessEvent(iocpObject, static_cast<IocpEvent*>(overlapped), numOfBytes);
        else if (errCode != WAIT_TIMEOUT)
            return false;
    }

    return true;
}

void IocpCore::ProcessEvent(IocpObject* obj, IocpEvent* event, int32 bytes)
{
    ASSERT_CRASH(obj != nullptr);
    ASSERT_CRASH(event != nullptr);

    IocpObjectRef ref = event->GetOwner();

    const bool measure = (GServerStats != nullptr);
    auto start = measure ? std::chrono::high_resolution_clock::now()
        : std::chrono::high_resolution_clock::time_point{};

    obj->Dispatch(event, bytes);

    if (measure)
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        GServerStats->iocp.iocpCallCount.fetch_add(1, std::memory_order_relaxed);
        GServerStats->iocp.totalProcessTimeUs.fetch_add(us, std::memory_order_relaxed);
    }
}