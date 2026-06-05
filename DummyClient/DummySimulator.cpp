#include "pch.h"
#include "DummySimulator.h"
#include "DummySession.h"
#include "Protocol/PacketUtils.h"

Vector3 DummyGroup::RandomMapPoint()
{
    std::uniform_real_distribution<float> dist(MAP_MIN, MAP_MAX);
    return { dist(_rng), 0.f, dist(_rng) };
}

uint32 DummyGroup::RandomChatDelayMs()
{
    std::uniform_int_distribution<uint32> dist(CHAT_MIN_MS, CHAT_MAX_MS);
    return dist(_rng);
}

bool DummyGroup::RollWorldChat()
{
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(_rng) < WORLD_CHAT_CHANCE;
}

void DummyGroup::AddAgent(DummySessionRef session, Vector3 spawnPos)
{
    auto now = std::chrono::steady_clock::now();

    uint32 offsetMs = static_cast<uint32>(_agents.size()) % TICK_MS;

    AgentState agent;
    agent.session = session;
    agent.pos = { spawnPos.x, 0.f, spawnPos.z };
    agent.target = RandomMapPoint();
    agent.speed = MOVE_SPEED;
    agent.nextMoveTime = now + std::chrono::milliseconds(offsetMs);
    agent.nextChatTime = now + std::chrono::milliseconds(RandomChatDelayMs());

    _agents.push_back(std::move(agent));
    _count.store(static_cast<int32>(_agents.size()), std::memory_order_relaxed);
}

void DummyGroup::RemoveSome(int32 n)
{
    int32 k = std::min(n, static_cast<int32>(_agents.size()));
    for (int32 i = 0; i < k; i++)
    {
        if (auto session = _agents.back().session.lock())
            session->Disconnect();
        _agents.pop_back();
    }
    _count.store(static_cast<int32>(_agents.size()), std::memory_order_relaxed);
}

void DummyGroup::Tick()
{
    if (_stopped.load(std::memory_order_relaxed))
        return;

    DoTimer(GROUP_TICK_MS, &DummyGroup::Tick);   // 다음 틱 재예약

    auto        now = std::chrono::steady_clock::now();
    const float tickSec = TICK_MS / 1000.f;      // 이동 1회당 델타(=0.1s)

    _agents.erase(
        std::remove_if(_agents.begin(), _agents.end(),
            [](const AgentState& a) { return a.session.expired(); }),
        _agents.end());

    for (AgentState& agent : _agents)
    {
        DummySessionRef session = agent.session.lock();
        if (!session) continue;

        if (now >= agent.nextMoveTime)
        {
            Vector3 toTarget = agent.target - agent.pos;
            float   distSq = toTarget.LengthSq();

            if (distSq <= ARRIVE_DIST_SQ)
            {
                agent.pos = agent.target;
                agent.target = RandomMapPoint();
                toTarget = agent.target - agent.pos;
                distSq = toTarget.LengthSq();
            }

            Vector3 dir = toTarget.Normalized();
            float   step = agent.speed * tickSec;
            float   dist = sqrtf(distSq);

            if (dist <= step)
                agent.pos = agent.target;
            else
                agent.pos = agent.pos + dir * step;

            float yaw = atan2f(dir.x, dir.z) * RAD2DEG;
            if (yaw < 0.f) yaw += 360.f;

            Vector3 vel = dir * agent.speed;

            Protocol::CMove pkt;
            auto* info = pkt.mutable_pos_info();
            info->mutable_pos()->set_x(agent.pos.x);
            info->mutable_pos()->set_y(agent.pos.y);
            info->mutable_pos()->set_z(agent.pos.z);
            info->set_yaw(yaw);
            info->set_state(Protocol::MOVING);
            info->mutable_velocity()->set_x(vel.x);
            info->mutable_velocity()->set_y(vel.y);
            info->mutable_velocity()->set_z(vel.z);

            session->Send(MakeSendBuffer<Protocol::C_MOVE>(pkt));   // 락 없으니 인라인 전송
            agent.nextMoveTime += std::chrono::milliseconds(TICK_MS);
        }

        if (now >= agent.nextChatTime)
        {
            bool world = RollWorldChat();

            Protocol::CChat chatPkt;
            chatPkt.set_chat_type(world ? Protocol::CHAT_WORLD : Protocol::CHAT_NEAR);
            chatPkt.set_chat(session->GetPlayerName() + " #" + std::to_string(agent.chatSeq++));

            session->Send(MakeSendBuffer<Protocol::C_CHAT>(chatPkt));
            agent.nextChatTime = now + std::chrono::milliseconds(RandomChatDelayMs());
        }
    }

    _count.store(static_cast<int32>(_agents.size()), std::memory_order_relaxed);
}

void DummySimulator::Start()
{
    for (int32 i = 0; i < GROUP_COUNT; i++)
    {
        auto group = MakeShared<DummyGroup>();
        _groups.push_back(group);

        uint32 offset = (DummyGroup::TICK_MS * i) / GROUP_COUNT;  // 그룹별 최초 발화 스태거
        group->DoTimer(offset, &DummyGroup::Tick);
    }
}

void DummySimulator::Stop()
{
    for (auto& group : _groups)
        group->Stop();
}

void DummySimulator::AddSession(DummySessionRef session, Vector3 spawnPos)
{
    if (_groups.empty()) return;

    auto group = _groups[_rr.fetch_add(1, std::memory_order_relaxed) % GROUP_COUNT];
    group->DoAsync(MakeJob([group, session, spawnPos]()      // 그룹 잡으로 안전 추가
        {
            group->AddAgent(session, spawnPos);
        }));
}

void DummySimulator::RemoveN(int32 n)
{
    if (_groups.empty() || n <= 0) return;

    int32 per = (n + GROUP_COUNT - 1) / GROUP_COUNT;
    for (auto& group : _groups)
    {
        if (n <= 0) break;
        int32 k = std::min(per, n);
        group->DoAsync(MakeJob([group, k]() { group->RemoveSome(k); }));
        n -= k;
    }
}

int32 DummySimulator::GetActiveCount()
{
    int32 sum = 0;
    for (auto& group : _groups)
        sum += group->Count();
    return sum;   // 근사치(원자 합) — 부하 툴 용도엔 충분
}