using Google.Protobuf;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PacketMessage
{
    public ushort Id { get; set; }
    public IMessage Message { get; set; }
}

public class PacketQueue
{
    public static PacketQueue Instance { get; } = new PacketQueue();

    Queue<PacketMessage> _packetQueue = new Queue<PacketMessage>();
    object _lock = new object();

    List<PacketMessage> _popBuffer = new List<PacketMessage>();

    public void Push(ushort id, IMessage packet)
    {
        lock (_lock)
        {
            _packetQueue.Enqueue(new PacketMessage() { Id = id, Message = packet });
        }
    }

    public PacketMessage Pop()
    {
        lock (_lock)
        {
            if (_packetQueue.Count == 0)
                return null;

            return _packetQueue.Dequeue();
        }
    }

    public List<PacketMessage> PopAll()
    {
        _popBuffer.Clear();

        lock (_lock)
        {
            while (_packetQueue.Count > 0)
                _popBuffer.Add(_packetQueue.Dequeue());
        }

        return _popBuffer;
    }
}