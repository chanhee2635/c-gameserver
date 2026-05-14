#include "pch.h"
#include "Service.h"
#include "DummySession.h"
#include "ThreadManager.h"

enum { WORKER_TICK = 64 };

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
    CoreGlobal::Init();

    httplib::Client cli("127.0.0.1", 5245);
    nlohmann::json body = { {"id", "test"}, {"pw", "1234"} };

    auto res = cli.Post("/api/auth/login",
        body.dump(), "application/json");

    if (!res || res->status != 200)
    {
        std::cout << "로그인 실패" << std::endl;
        return -1;
    }

    auto resJson = nlohmann::json::parse(res->body);
    string authToken = resJson["authToken"].get<string>();
    std::cout << "로그인 성공, token: " << authToken << std::endl;

    // 2. GameServer TCP 연결
    auto session = MakeShared<DummySession>();
    session->SetAuthToken(authToken);

    auto service = MakeShared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        MakeShared<IocpCore>(),
        [session]() { return session; },
        1
    );

    ASSERT_CRASH(service->Start());

    for (int32 i = 0; i < 2; i++)
    {
        GThread->Launch(ThreadType::WORKER, [&service]()
            {
                DoWorkerJob(service);
            });
    }

    GThread->InitMainThread(ThreadType::MONITOR);
    if (GServerStats)
        GServerStats->RunUpdateLoop();

    CoreGlobal::Clear();
    return 0;
}