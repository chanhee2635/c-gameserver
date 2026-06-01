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
        Vector<uint64> reservedExpiry;   // �߱������� ���� ���Ӽ��� �������� ����(���� tick)
    };

    HashMap<int32, ServerQueue> _queues;

    // Admission runs every TICK_MS (fine-grained) to spread player spawns evenly
    // across game-server scene ticks instead of bursting BATCH_PER_TICK at once.
    // Queue-position status is broadcast only every STATUS_EVERY ticks (~1s).
    uint32 _tickCounter = 0;
    static constexpr uint32 STATUS_EVERY = 10;   // 100ms * 10 = ~1s status cadence

    static constexpr uint32 TICK_MS        = 100;   // ���� ���� �ֱ�
    static constexpr uint64 RESERVE_TTL_MS = 8000;   // ��ū �߱� �� ���� ����
    static constexpr int32  BATCH_PER_TICK = 5;     // �ʴ� �ִ� ����(���� ��źȭ)
};