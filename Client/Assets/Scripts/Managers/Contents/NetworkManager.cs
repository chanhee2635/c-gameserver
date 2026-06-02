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

    public void Clear()
    {
        _clockSyncEnabled = false;
        _session.Disconnect();
    }

    public void EnqueueMainThread(Action action) => _mainThreadActions.Enqueue(action);

    public void Update()
    {
        while (_mainThreadActions.TryDequeue(out var action))
            action();

        UpdateClockSync();
    }

    // ── Clock sync (SNTP-style) ───────────────────────────────────────────────
    // Convert local time to server time so the server can measure true latency
    // (network + queue) for dead reckoning. Runs entirely on the main thread
    // (packets are dispatched from PacketQueue.Flush on the main thread).
    private const float SYNC_INTERVAL_FAST = 0.25f; // burst until first lock (~2s for 8)
    private const float SYNC_INTERVAL_SLOW = 5f;    // steady-state resync (tracks drift)
    private const int   SYNC_BURST_COUNT   = 8;
    private const int   SYNC_WINDOW_RESET  = 12;    // reopen RTT window every ~1min

    private bool   _clockSyncEnabled;
    private bool   _clockSynced;
    private double _clockOffsetMs;     // serverClock - clientClock
    private double _minRttMs = double.MaxValue;
    private int    _syncSent;
    private float  _syncTimer;

    private static double LocalMs() => Time.timeAsDouble * 1000.0;

    public void StartClockSync()
    {
        _clockSyncEnabled = true;
        _clockSynced = false;
        _minRttMs = double.MaxValue;
        _syncSent = 0;
        _syncTimer = 0f;
    }

    // Send time in server-clock estimate; 0 until first lock (server then skips DR).
    public ulong ServerNowMs() => _clockSynced ? (ulong)(LocalMs() + _clockOffsetMs) : 0UL;

    public void OnTimeSync(ulong clientSendMs, ulong serverTimeMs)
    {
        double now = LocalMs();
        double rtt = now - clientSendMs;
        if (rtt < 0) return;

        // Server clock at the round-trip midpoint corresponds to serverTimeMs.
        double offset = serverTimeMs - (clientSendMs + now) * 0.5;

        // Adopt the lowest-RTT sample's offset (least dispatch/queue jitter => most accurate).
        if (rtt < _minRttMs)
        {
            _minRttMs = rtt;
            _clockOffsetMs = offset;
            _clockSynced = true;
        }
    }

    private void UpdateClockSync()
    {
        if (!_clockSyncEnabled) return;

        _syncTimer -= Time.unscaledDeltaTime;
        if (_syncTimer > 0f) return;

        bool locked = _clockSynced && _syncSent >= SYNC_BURST_COUNT;
        _syncTimer = locked ? SYNC_INTERVAL_SLOW : SYNC_INTERVAL_FAST;
        _syncSent++;

        if (locked && (_syncSent % SYNC_WINDOW_RESET) == 0)
            _minRttMs = double.MaxValue;   // reopen window to re-acquire drift

        Send(new Protocol.CTimeSync { ClientSendMs = (ulong)LocalMs() });
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
