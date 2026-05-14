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

    private readonly ServerSession          _session           = new();
    private readonly ConcurrentQueue<Action> _mainThreadActions = new();

    public void ConnectToGameServer(ServerInfo info)
    {
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
}
