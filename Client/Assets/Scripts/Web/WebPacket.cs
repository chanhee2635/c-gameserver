using System;
using System.Collections.Generic;

[Serializable]
public class ServerInfo
{
    public int Id;
    public string ServerName;
    public string IpAddress;
    public int Port;
    public int CurrentCount;
    public int MaxCount;
}

[Serializable]
public class CreateAccountReq
{
    public string AccountName;
    public string Password;
}

[Serializable]
public class CreateAccountRes
{
    public bool Success;
}

[Serializable]
public class LoginAccountReq
{
    public string AccountName;
    public string Password;
}

[Serializable]
public class LoginAccountRes
{
    public bool Success;
    public int AccountId;
    public string QueueToken;   
    public string GateIp;
    public int GatePort;
    public List<ServerInfo> ServerList = new();
}