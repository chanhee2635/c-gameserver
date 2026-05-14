using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using Google.Protobuf;
using UnityEngine;

internal sealed class Packet<T> : IPacket where T : IMessage
{
    public ushort ProtocolId { get; }
    readonly T _msg;
    readonly Action<T> _handler;

    public Packet(ushort id, T msg, Action<T> handler)
    { ProtocolId = id; _msg = msg; _handler = handler; }

    public void Dispatch() => _handler(_msg);
}

public sealed partial class PacketManager
{
    static readonly PacketManager _instance = new();
    public static PacketManager Instance => _instance;

    PacketManager() => Register();

    readonly Dictionary<ushort, Action<ArraySegment<byte>>> _onRecv = new();

    public Action<IPacket> CustomHandler { get; set; }

    private void AddHandler<T>(ushort id, Action<T> handler) where T : IMessage, new()
    {
        _onRecv[id] = seg =>
        {
            T pkt = new T();
            pkt.MergeFrom(seg.Array, seg.Offset + 4, seg.Count - 4); // 헤더 4바이트 스킵

            if (CustomHandler != null)
                CustomHandler(new Packet<T>(id, pkt, handler));
            else
                handler(pkt);
        };
    }

    public void OnRecvPacket(ArraySegment<byte> buffer)
    {
        if (buffer.Count < 4) return;
        ushort id = BinaryPrimitives.ReadUInt16LittleEndian(
            new ReadOnlySpan<byte>(buffer.Array, buffer.Offset + 2, 2));

        if (_onRecv.TryGetValue(id, out var action))
            action(buffer);
        else
            Debug.LogWarning($"[PacketManager] Unknown packet id: {id}");
    }

    partial void Register(); 
}