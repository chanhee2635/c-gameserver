#include "pch.h"
#include "DummySimulator.h"
#include "DummySession.h"
#include "Protocol/PacketUtils.h"

DummySimulator* GDummySimulator = nullptr;

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

void DummySimulator::AddSession(DummySessionRef session, Vector3 spawnPos)
{
    static constexpr float GOLDEN_ANGLE = 137.508f;
    using clock = std::chrono::steady_clock;

    LockGuard guard(_lock);

    float startAngle = fmodf(GOLDEN_ANGLE * static_cast<float>(_agents.size()), 360.f);
    float startRad   = startAngle * DEG2RAD;

    Vector3 center = {
        spawnPos.x - MOVE_RADIUS * sinf(startRad),
        0.f,
        spawnPos.z - MOVE_RADIUS * cosf(startRad)
    };

    uint32 offsetMs = static_cast<uint32>(_agents.size()) % TICK_MS;
    auto nextSend = s_baseTime + std::chrono::milliseconds(offsetMs);
    auto now = clock::now();
    while (nextSend <= now)
        nextSend += std::chrono::milliseconds(TICK_MS);

    _agents.push_back({ session, center, startAngle, 2.0f, nextSend });
}

void DummySimulator::Run()
{
    using clock = std::chrono::steady_clock;

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
                if (now < agent.nextSendTime) continue;

                DummySessionRef session = agent.session.lock();
                if (!session)
                {
                    agent.nextSendTime = now + std::chrono::milliseconds(TICK_MS);
                    continue;
                }

                float prevRad = agent.angle * DEG2RAD;
                Vector3 prevPos = {
                    agent.center.x + MOVE_RADIUS * sinf(prevRad),
                    agent.center.y,
                    agent.center.z + MOVE_RADIUS * cosf(prevRad)
                };

                const float tickSec = TICK_MS / 1000.f;
                const float arcLen = agent.speed * tickSec;
                const float angleStep = (arcLen / (2.f * PI * MOVE_RADIUS)) * 360.f;

                agent.angle += angleStep;
                if (agent.angle >= 360.f) agent.angle -= 360.f;

                float rad = agent.angle * DEG2RAD;
                Vector3 pos = {
                    agent.center.x + MOVE_RADIUS * sinf(rad),
                    agent.center.y,
                    agent.center.z + MOVE_RADIUS * cosf(rad)
                };

                Vector3 dir = { pos.x - prevPos.x, 0.f, pos.z - prevPos.z };
                float yaw = atan2f(dir.x, dir.z) * RAD2DEG;
                if (yaw < 0.f) yaw += 360.f;

                Protocol::CMove pkt;
                auto* info = pkt.mutable_pos_info();
                info->mutable_pos()->set_x(pos.x);
                info->mutable_pos()->set_y(pos.y);
                info->mutable_pos()->set_z(pos.z);
                info->set_yaw(yaw);
                info->set_state(Protocol::MOVING);

                toSend.emplace_back(session, MakeSendBuffer<Protocol::C_MOVE>(pkt));

                agent.nextSendTime += std::chrono::milliseconds(TICK_MS);
            }
        }  

        for (auto& [session, buffer] : toSend)
            session->Send(buffer);

        LFrameAllocator->Clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}