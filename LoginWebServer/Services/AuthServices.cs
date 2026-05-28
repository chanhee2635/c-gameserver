using LoginWebServer.Models;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;

namespace LoginWebServer.Services;

public class AuthService
{
    private readonly AppDbContext _db;
    private readonly IConnectionMultiplexer _redis;
    private readonly string _gateIp;
    private readonly int _gatePort;

    private const int TOKEN_TTL_SECONDS = 600;
    private const int MIN_PASSWORD_LENGTH = 8;

    public AuthService(AppDbContext db, IConnectionMultiplexer redis, IConfiguration config)
    {
        _db = db;
        _redis = redis;
        _gateIp = config["Gateway:Ip"] ?? "127.0.0.1";
        _gatePort = int.TryParse(config["Gateway:Port"], out var p) ? p : 6666;
    }

    public async Task<CreateAccountRes> CreateAccountAsync(CreateAccountReq req)
    {
        var res = new CreateAccountRes();

        if (string.IsNullOrWhiteSpace(req.Password) || req.Password.Length < MIN_PASSWORD_LENGTH)
        {
            res.Success = false;
            return res;
        }

        bool exists = await _db.Accounts.AnyAsync(a => a.AccountName == req.AccountName);
        if (exists)
        {
            res.Success = false;
            return res;
        }

        _db.Accounts.Add(new AccountDb
        {
            AccountName = req.AccountName,
            PasswordHash = BCrypt.Net.BCrypt.HashPassword(req.Password)
        });

        try
        {
            await _db.SaveChangesAsync();
            res.Success = true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[CreateAccount] Error: {ex.Message}");
            res.Success = false;
        }

        return res;
    }

    public async Task<LoginAccountRes> LoginAccountAsync(LoginAccountReq req)
    {
        var res = new LoginAccountRes();

        AccountDb? account = await _db.Accounts
            .AsNoTracking()
            .Where(a => a.AccountName == req.AccountName)
            .FirstOrDefaultAsync();

        if (account == null || !BCrypt.Net.BCrypt.Verify(req.Password, account.PasswordHash))
        {
            res.Success = false;
            return res;
        }

        var redisDb = _redis.GetDatabase();

        string queueToken = Guid.NewGuid().ToString("N");
        await redisDb.StringSetAsync($"qsession:{queueToken}", account.AccountId.ToString(),
                                     TimeSpan.FromSeconds(TOKEN_TTL_SECONDS));

        res.Success = true;
        res.AccountId = account.AccountId;
        res.QueueToken = queueToken;
        res.GateIp = _gateIp;
        res.GatePort = _gatePort;
        res.ServerList = await GetServerListAsync(redisDb);

        return res;
    }

    private async Task<List<ServerInfo>> GetServerListAsync(IDatabase redisDb)
    {
        var list = new List<ServerInfo>();

        RedisValue[] ids = await redisDb.SetMembersAsync("server_ids");

        foreach (var id in ids)
        {
            HashEntry[] fields = await redisDb.HashGetAllAsync($"servers:{id}");
            if (fields.Length == 0)
            {
                await redisDb.SetRemoveAsync("server_ids", id);
                continue;
            }

            var dict = fields.ToDictionary(f => f.Name.ToString(), f => f.Value.ToString());

            list.Add(new ServerInfo
            {
                Id = int.Parse(id.ToString()),
                ServerName = dict.GetValueOrDefault("name", ""),
                IpAddress = dict.GetValueOrDefault("ip", ""),
                Port = int.Parse(dict.GetValueOrDefault("port", "0")),
                CurrentCount = int.Parse(dict.GetValueOrDefault("current", "0")),
                MaxCount = int.Parse(dict.GetValueOrDefault("max", "0"))
            });
        }

        return list;
    }
}