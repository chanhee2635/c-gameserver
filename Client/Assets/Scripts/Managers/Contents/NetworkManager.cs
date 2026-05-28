using System;
using System.Collections.Concurrent;
using System.Net;
using Google.Protobuf;
using ServerCore;
using UnityEngine;

public class NetworkManager
{
    // 로그인 서버 인증 후 발급된 토큰 — TCP 연결 후 C_AUTH_TOKEN으로 전송
    public int    AccountId { get; set; }
    public string AuthToken { get; set; }
    public string QueueToken { get; set; }
    public string GateIp { get; set; }
    public int GatePort { get; set; }
    public int QueueServerId => _queueServerId;

    public event Action<int, int> OnQueueStatus;    
    public event Action OnQueueAdmitted;  
    public event Action<string> OnQueueRejected;  

    private int _queueServerId;

    private GateClientSession       _gateSession       = new();
    private ServerSession           _session           = new();
    private ConcurrentQueue<Action> _mainThreadActions = new();

    public void ConnectToGateway(int serverId)
    {
        _queueServerId = serverId;
        _gateSession = new GateClientSession();

        var endPoint = new IPEndPoint(IPAddress.Parse(GateIp), GatePort);
        new Connector().Connect(endPoint, () => _gateSession, 1);
    }

    public void LeaveQueue()
    {
        if (_gateSession == null) return;
        _gateSession.SendLeave();
        _gateSession.Disconnect();
        _gateSession = null;
    }

    public void ConnectToGameServer(ServerInfo info)
    {
        _session = new ServerSession();

        while (_mainThreadActions.TryDequeue(out _)) { }

        var endPoint = new IPEndPoint(IPAddress.Parse(info.IpAddress), info.Port);
        new Connector().Connect(endPoint, () => _session, 1);
    }

    public void Send(IMessage packet) => _session.Send(packet);

    public void Clear() => _session.Disconnect();

    public void EnqueueMainThread(Action action) => _mainThreadActions.Enqueue(action);

    public void Update()
    {
        while (_mainThreadActions.TryDequeue(out var action))
            action();
    }

    public void RaiseQueueStatus(int pos, int total)
    => EnqueueMainThread(() => OnQueueStatus?.Invoke(pos, total));

    public void RaiseQueueRejected(string reason)
        => EnqueueMainThread(() => OnQueueRejected?.Invoke(reason));

    public void HandleAdmitted(string authToken, string gameIp, int gamePort)
    {
        EnqueueMainThread(() =>
        {
            AuthToken = authToken;        
            OnQueueAdmitted?.Invoke();   
            _gateSession = null;

            var info = new ServerInfo { IpAddress = gameIp, Port = gamePort };
            ConnectToGameServer(info);    
        });
    }
}
