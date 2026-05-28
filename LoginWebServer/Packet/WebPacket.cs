using System;
using System.Collections.Generic;

public class ServerInfo
{
    public int Id { get; set; }
    public string ServerName { get; set; }
    public string IpAddress { get; set; }
    public int Port { get; set; }
    public int CurrentCount { get; set; }
    public int MaxCount { get; set; }
}

public class CreateAccountReq
{
    public string AccountName { get; set; }
    public string Password { get; set; }
}
public class CreateAccountRes
{
    public bool Success { get; set; }
}

public class LoginAccountReq
{
    public string AccountName { get; set; }
    public string Password { get; set; }
}

public class LoginAccountRes
{
    public bool Success { get; set; }
    public int AccountId { get; set; }
    public string QueueToken { get; set; }  
    public string GateIp { get; set; }
    public int GatePort { get; set; }
    public List<ServerInfo> ServerList { get; set; } = new();
}