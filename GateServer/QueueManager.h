#pragma once
#include "GateSession.h"

class QueueManager : public JobQueue
{
public:
    void Start();
    void Enqueue(GateSessionRef session, uint64 accountId, int32 serverId);

private:
    void Tick();

    struct Waiter
    {
        GateSessionWeakRef session;
        uint64             accountId = 0;
    };
    struct ServerQueue
    {
        Deque<Waiter>  waiters;
        Vector<uint64> reservedExpiry;   // 발급했지만 아직 게임서버 미접속인 예약(만료 tick)
    };

    HashMap<int32, ServerQueue> _queues;

    static constexpr uint32 TICK_MS        = 1000;   // 순번 갱신 주기
    static constexpr uint64 RESERVE_TTL_MS = 8000;   // 토큰 발급 후 접속 유예
    static constexpr int32  BATCH_PER_TICK = 50;     // 초당 최대 입장(폭주 평탄화)
};