using System;
using System.Collections.Concurrent;

public interface IPacket
{
    ushort ProtocolId { get; }
    void Dispatch();  // ← 추가
}

public sealed class PacketQueue
{
    public static readonly PacketQueue Instance = new();
    private readonly ConcurrentQueue<IPacket> _queue = new();

    public void Push(IPacket packet) => _queue.Enqueue(packet);

    // 메인 스레드에서 호출. Dispatch()가 핸들러까지 직접 실행.
    public void Flush(int maxCount = int.MaxValue)
    {
        int count = 0;
        while (count++ < maxCount && _queue.TryDequeue(out IPacket pkt))
            pkt.Dispatch();
    }

    public int Count => _queue.Count;
}