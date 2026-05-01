#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"
#include "ServerStats.h"

/*-----------------
     IocpCore
-----------------*/

IocpCore::IocpCore()
{
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
    ::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
    return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, /*key*/0, 0);
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
    DWORD numOfBytes = 0;
    ULONG_PTR key = 0;
    IocpEvent* iocpEvent = nullptr;

    if (::GetQueuedCompletionStatus(
        _iocpHandle,
        OUT &numOfBytes,
        OUT &key,
        OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent),
        timeoutMs))
    {
        // ── IOCP 처리 시간 측정 → ServerStats.iocp 기록 ──────────────
        LARGE_INTEGER freq, t0, t1;
        ::QueryPerformanceFrequency(&freq);
        ::QueryPerformanceCounter(&t0);

        IocpObjectRef iocpObject = iocpEvent->owner;
        iocpObject->Dispatch(iocpEvent, numOfBytes);

        ::QueryPerformanceCounter(&t1);
        uint64 elapsedUs = static_cast<uint64>((t1.QuadPart - t0.QuadPart) * 1000000
                                               / freq.QuadPart);

        auto& s = ServerStats::Get().iocp;
        s.iocpCallCount.fetch_add(1, std::memory_order_relaxed);
        s.totalProcessTimeUs.fetch_add(elapsedUs, std::memory_order_relaxed);

#ifdef _DEBUG
        if (elapsedUs > 10000)  // 10ms 이상 → 경고
            printf("[IocpCore] Slow dispatch: type=%d elapsed=%llu us\n",
                   (int)iocpEvent->type, elapsedUs);
#endif
    }
    else
    {
        int32 errCode = ::WSAGetLastError();
        switch (errCode)
        {
        case WAIT_TIMEOUT:
            return false;
        default:
            // 소켓 오류로 인한 실패 완료 (Disconnect 등) → 동일하게 Dispatch
            if (iocpEvent != nullptr && iocpEvent->owner != nullptr)
            {
                IocpObjectRef iocpObject = iocpEvent->owner;
                iocpObject->Dispatch(iocpEvent, numOfBytes);
            }
            break;
        }
    }

    return true;
}
