#pragma once
#include <optional>
#include <sw/redis++/redis++.h>

struct ServerSlot
{
    string ip;
    int32  port = 0;
    int32  current = 0;
    int32  max = 0;
};

class GateRedis
{
public:
    bool Connect(const string& host, int32 port);

    std::optional<uint64> ResolveQueueToken(const string& queueToken); 
    bool                  GetServerInfo(int32 serverId, ServerSlot& out);
    std::string           IssueAuthToken(uint64 accountId);            

private:
    std::unique_ptr<sw::redis::Redis> _redis;
};