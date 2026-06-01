#include "pch.h"
#include "QueueManager.h"
#include "PacketUtils.h"
#include "GateRedis.h"
#include "GateGlobal.h"

void QueueManager::Start()
{
    DoTimer(TICK_MS, [this]() { Tick(); });
}

void QueueManager::Enqueue(GateSessionRef session, uint64 accountId, int32 serverId)
{
    // �� ������� �Ѱ� ����ȭ(�� ���ʿ�)
    DoAsync([this, session, accountId, serverId]()
        {
            _queues[serverId].waiters.push_back(Waiter{ session, accountId });
        });
}

void QueueManager::Tick()
{
    const uint64 now = ::GetTickCount64();
    const bool sendStatus = (++_tickCounter % STATUS_EVERY == 0);   // throttle status broadcast to ~1s

    for (auto& [serverId, q] : _queues)
    {
        // 1) ���� ����� ����
        std::erase_if(q.waiters, [](const Waiter& w)
            {
                GateSessionRef s = w.session.lock();
                return (s == nullptr) || (s->IsConnected() == false);
            });

        // 2) ����� ���� ȸ��
        std::erase_if(q.reservedExpiry, [now](uint64 exp) { return exp <= now; });

        // 3) ���Ӽ��� �뷮 ��ȸ
        ServerSlot slot;
        if (GGateRedis->GetServerInfo(serverId, slot) == false)
            continue;   // ���� �ٿ�/�̵�� �� ���� tick ��õ�

        const int32 occupancy = slot.current + static_cast<int32>(q.reservedExpiry.size());
        int32 freeSlots = slot.max - occupancy;
        if (freeSlots < 0) freeSlots = 0;

        // 4) �տ������� ���� �㰡
        const int32 admit = std::min({ freeSlots,
                                       static_cast<int32>(q.waiters.size()),
                                       BATCH_PER_TICK });
        for (int32 i = 0; i < admit; ++i)
        {
            Waiter w = q.waiters.front();
            q.waiters.pop_front();

            GateSessionRef s = w.session.lock();
            if (!s || !s->IsConnected()) continue;   // ��� ���� �� ���� ���� ���� ��ŵ

            const string token = GGateRedis->IssueAuthToken(w.accountId);

            Protocol::SQAdmitted res;
            res.set_auth_token(token);
            res.set_game_ip(slot.ip);
            res.set_game_port(slot.port);
            s->Send(MakeSendBuffer<Protocol::S_Q_ADMITTED>(res));

            q.reservedExpiry.push_back(now + RESERVE_TTL_MS);
        }

        // 5) ���� ����ڿ��� ���� ����
        if (!sendStatus) continue;   // admit every tick, but broadcast queue position only every STATUS_EVERY ticks
        const int32 total = static_cast<int32>(q.waiters.size());
        int32 idx = 0;
        for (Waiter& w : q.waiters)
        {
            ++idx;
            GateSessionRef s = w.session.lock();
            if (!s) continue;

            Protocol::SQStatus st;
            st.set_position(idx);
            st.set_total(total);
            st.set_eta_sec(0);   // TODO: �巹�� �ӵ� ��� ����
            s->Send(MakeSendBuffer<Protocol::S_Q_STATUS>(st));
        }
    }

    DoTimer(TICK_MS, [this]() { Tick(); });   // �翹��
}