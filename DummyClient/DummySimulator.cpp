#include "pch.h"
#include "DummySimulator.h"
#include "DummySession.h"
#include "Protocol/PacketUtils.h"

void DummySimulator::Start()
{
    _running = true;
    GThread->Launch(ThreadType::WORKER, [this]() { Run(); });
}

void DummySimulator::Stop()
{
    _running = false;
}

int32 DummySimulator::GetActiveCount()
{
    LockGuard guard(_lock);
    return static_cast<int32>(_agents.size());
}

void DummySimulator::RemoveN(int32 n)
{
    LockGuard guard(_lock);
    int32 removeCount = std::min(n, static_cast<int32>(_agents.size()));
    for (int32 i = 0; i < removeCount; i++)
    {
        if (auto session = _agents.back().session.lock())
            session->Disconnect();
        _agents.pop_back();
    }
}

Vector3 DummySimulator::RandomMapPoint()
{
    std::uniform_real_distribution<float> dist(MAP_MIN, MAP_MAX);
    return { dist(_rng), 0.f, dist(_rng) };
}

uint32 DummySimulator::RandomChatDelayMs()
{
    std::uniform_int_distribution<uint32> dist(CHAT_MIN_MS, CHAT_MAX_MS);
    return dist(_rng);
}

bool DummySimulator::RollWorldChat()
{
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(_rng) < WORLD_CHAT_CHANCE;
}

void DummySimulator::AddSession(DummySessionRef session, Vector3 spawnPos)
{
    using clock = std::chrono::steady_clock;

    LockGuard guard(_lock);

    uint32 offsetMs  = static_cast<uint32>(_agents.size()) % TICK_MS;
    auto   now       = clock::now();
    auto   nextMove  = s_baseTime + std::chrono::milliseconds(offsetMs);
    while (nextMove <= now)
        nextMove += std::chrono::milliseconds(TICK_MS);

    AgentState agent;
    agent.session      = session;
    agent.pos          = { spawnPos.x, 0.f, spawnPos.z };
    agent.target       = RandomMapPoint();
    agent.speed        = MOVE_SPEED;
    agent.nextMoveTime = nextMove;
    agent.nextChatTime = now + std::chrono::milliseconds(RandomChatDelayMs());

    _agents.push_back(std::move(agent));
}

void DummySimulator::Run()
{
    using clock = std::chrono::steady_clock;
    const float tickSec = TICK_MS / 1000.f;

    while (_running)
    {
        LEndTickCount = ::GetTickCount64() + TICK_MS;
        auto now = clock::now();

        Vector<std::pair<DummySessionRef, SendBufferRef>> toSend;
        {
            LockGuard guard(_lock);

            _agents.erase(
                std::remove_if(_agents.begin(), _agents.end(),
                    [](const AgentState& a) { return a.session.expired(); }),
                _agents.end());

            for (AgentState& agent : _agents)
            {
                DummySessionRef session = agent.session.lock();
                if (!session) continue;

                // ---- 이동: 목표를 향해 직선 전진, 도달하면 새 목표 선택 ----
                if (now >= agent.nextMoveTime)
                {
                    Vector3 toTarget = agent.target - agent.pos;
                    float   distSq   = toTarget.LengthSq();

                    if (distSq <= ARRIVE_DIST_SQ)
                    {
                        agent.pos    = agent.target;
                        agent.target = RandomMapPoint();
                        toTarget     = agent.target - agent.pos;
                        distSq       = toTarget.LengthSq();
                    }

                    Vector3 dir  = toTarget.Normalized();
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

                    toSend.emplace_back(session, MakeSendBuffer<Protocol::C_MOVE>(pkt));

                    agent.nextMoveTime += std::chrono::milliseconds(TICK_MS);
                }

                // ---- 채팅: 일정 간격으로 근거리(가끔 전역) 전송 ----
                if (now >= agent.nextChatTime)
                {
                    bool world = RollWorldChat();

                    Protocol::CChat chatPkt;
                    chatPkt.set_chat_type(world ? Protocol::CHAT_WORLD : Protocol::CHAT_NEAR);
                    chatPkt.set_chat(session->GetPlayerName() + " #" + std::to_string(agent.chatSeq++));

                    toSend.emplace_back(session, MakeSendBuffer<Protocol::C_CHAT>(chatPkt));

                    agent.nextChatTime = now + std::chrono::milliseconds(RandomChatDelayMs());
                }
            }
        }

        for (auto& [session, buffer] : toSend)
            session->Send(buffer);

        LFrameAllocator->Clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
