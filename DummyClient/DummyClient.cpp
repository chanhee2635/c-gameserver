#include "pch.h"
#include "Service.h"
#include "DummySession.h"
#include "ThreadManager.h"
#include "DummySimulator.h"
#include "DummyTypes.h"
#include "DummyNet.h"
#include "DummyGateSession.h"
#include "DummyGlobal.h"

enum { WORKER_TICK = 16 };

static constexpr int32  LOGIN_BATCH    = 20;
static constexpr int32  BATCH_DELAY_MS = 100;
static constexpr int32  LOGIN_PORT     = 5245;
static const     string SERVER_IP      = "127.0.0.1";
static const     string PASSWORD       = "test1234";

void DoWorkerJob(ClientServiceRef& service)
{
    while (true)
    {
        LEndTickCount = ::GetTickCount64() + WORKER_TICK;

        service->IocpDispatch();

        GJobTimer->Distribute(::GetTickCount64());

        ThreadManager::DoGlobalQueueWork();

        LFrameAllocator->Clear();
    }
}

int main()
{
    DummyGlobal::Init();

    ClientServiceRef service = DummyGlobal::GetService();

    const int32 workerCount = std::max(2, static_cast<int32>(Thread::hardware_concurrency()));
    for (int32 i = 0; i < workerCount; i++)
    {
        GThread->Launch(ThreadType::WORKER, [&service]()
        {
            DoWorkerJob(service);
        });
    }

    GThread->InitMainThread(ThreadType::MONITOR);
    GThread->Launch(ThreadType::MONITOR, []()
    {
        if (GServerStats)
            GServerStats->RunUpdateLoop();
    });

    Atomic<int32> nextAccountIdx = 0;

    auto tryLogin = [&](int32 accountIdx) -> DummyGateSessionRef
        {
            try
            {
                string account = "dummy_" + std::to_string(accountIdx);
                httplib::Client cli(SERVER_IP, LOGIN_PORT);
                cli.set_connection_timeout(10);
                cli.set_read_timeout(10);

                nlohmann::json body = { {"AccountName", account}, {"Password", PASSWORD} };
                auto res = cli.Post("/api/auth/login", body.dump(), "application/json");
                if (!res || res->status != 200)
                {
                    LOG_ERROR(L"Login failed: " + Utils::ToWString(account));
                    return nullptr;
                }

                auto   resJson = nlohmann::json::parse(res->body);
                string queueToken = resJson["QueueToken"].get<string>();   // ← AuthToken 아님

                auto s = MakeShared<DummyGateSession>();
                s->SetQueueToken(queueToken);
                s->SetServerId(1);
                s->SetAccountIdx(accountIdx);
                return s;
            }
            catch (...) { LOG_ERROR(L"Exception in login thread"); return nullptr; }
        };

    auto loginBatch = [&](int32 count, Vector<DummyGateSessionRef>& out, Mutex& outMutex)
    {
        for (int32 start = 0; start < count; start += LOGIN_BATCH)
        {
            int32          end     = std::min(start + LOGIN_BATCH, count);
            Vector<Thread> threads;

            for (int32 i = start; i < end; i++)
            {
                int32 accountIdx = nextAccountIdx++;
                threads.emplace_back([&, accountIdx]()
                {
                    if (auto session = tryLogin(accountIdx))
                    {
                        LockGuard guard(outMutex);
                        out.push_back(session);
                    }
                });
            }

            for (auto& t : threads) t.join();

            if (end < count)
                std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_DELAY_MS));
        }
    };

    static constexpr int32 MAX_LOGIN_RETRIES = 3;

    auto connectDummies = [&](int32 n)
    {
        Vector<DummyGateSessionRef> newSessions;
        Mutex newMutex;

        for (int32 attempt = 0; attempt < MAX_LOGIN_RETRIES; attempt++)
        {
            int32 needed = n - static_cast<int32>(newSessions.size());
            if (needed <= 0) break;

            loginBatch(needed, newSessions, newMutex);
        }

        if (newSessions.empty())
        {
            LOG_WARN(L"[Dummy] Login failed — no sessions connected");
            return;
        }

        int32 shortfall = n - static_cast<int32>(newSessions.size());
        if (shortfall > 0)
            LOG_WARN(L"[Dummy] " + std::to_wstring(shortfall) + L" logins failed after retries");

        for (auto& s : newSessions)
            GDummyNet->ConnectGate(s);
    };

    auto removeDummies = [&](int32 n)
    {
        GDummySimulator->RemoveN(n);
    };

    auto createAccounts = [&](int32 n)
    {
        for (int32 start = 0; start < n; start += LOGIN_BATCH)
        {
            int32          end = std::min(start + LOGIN_BATCH, n);
            Vector<Thread> threads;

            for (int32 i = start; i < end; i++)
            {
                threads.emplace_back([i]()
                    {
                        try
                        {
                            string account = "dummy_" + std::to_string(i);
                            httplib::Client cli(SERVER_IP, LOGIN_PORT);
                            cli.set_connection_timeout(10);
                            cli.set_read_timeout(10);
                            nlohmann::json body = { {"AccountName", account}, {"Password", PASSWORD} };
                            cli.Post("/api/auth/create", body.dump(), "application/json");
                        }
                        catch (...) { LOG_ERROR(L"Account create failed"); }
                    });
            }
            for (auto& t : threads) t.join();
            if (end < n) std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_DELAY_MS));
        }
        LOG_INFO(L"[Dummy] Created accounts dummy_0 ~ dummy_" + std::to_wstring(n - 1));
    };

    // 콘솔 입력 루프
    LOG_INFO(L"=== Dummy Client Console ===");

    wstring line;
    while (std::getline(std::wcin, line))
    {
        if (line.empty()) continue;
        if (line == L"q" || line == L"Q") break;

        try
        {
            if (line[0] == L'+')
            {
                int32 n = std::stoi(line.substr(1));
                if (n > 0) connectDummies(n);
            }
            else if (line[0] == L'-')
            {
                int32 n = std::stoi(line.substr(1));
                if (n > 0) removeDummies(n);
            }
            else if (line.rfind(L"setup ", 0) == 0)
            {
                int32 n = std::stoi(line.substr(6));
                if (n > 0) createAccounts(n);
            }
            else
            {
                int32 target  = std::stoi(line);
                int32 current = GDummySimulator->GetActiveCount();
                if (target > current)
                    connectDummies(target - current);
                else if (target < current)
                    removeDummies(current - target);
                else
                    LOG_INFO(L"[Dummy] No change. Active: " + std::to_wstring(current));
            }
        }
        catch (...)
        {
            LOG_WARN(L"[Dummy] Invalid input. Use +N / -N / N / q");
        }
    }

    DummyGlobal::Clear();
    return 0;
}
