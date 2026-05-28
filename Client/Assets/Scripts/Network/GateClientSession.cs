using System;
using System.Buffers.Binary;
using System.Net;
using Google.Protobuf;
using Protocol;
using ServerCore;
using UnityEngine;

public class GateClientSession : PacketSession
{
    private bool _admitted = false;

    public override void OnConnected(EndPoint endPoint)
    {
        var req = new CQEnter
        {
            QueueToken = Managers.Network.QueueToken,
            ServerId = Managers.Network.QueueServerId
        };
        Send(PacketUtils.MakeSendBuffer(req, (ushort)GateMsgId.CQEnter));
    }

    public override void OnDisconnected(EndPoint endPoint)
    {
        if (_admitted) return;   
        Managers.Network.RaiseQueueRejected("게이트웨이 연결 종료");
    }

    public override void OnRecvPacket(ArraySegment<byte> buffer)
    {
        ushort type = BinaryPrimitives.ReadUInt16LittleEndian(new ReadOnlySpan<byte>(buffer.Array, buffer.Offset + 2, 2));
        int off = buffer.Offset + 4;     
        int len = buffer.Count - 4;

        switch ((GateMsgId)type)
        {
            case GateMsgId.SQEnter:
                {
                    var pkt = new SQEnter();
                    pkt.MergeFrom(buffer.Array, off, len);
                    if (!pkt.Success)
                    {
                        Managers.Network.RaiseQueueRejected(pkt.Reason);
                        Disconnect();
                    }
                    break;
                }
            case GateMsgId.SQStatus:
                {
                    var pkt = new SQStatus();
                    pkt.MergeFrom(buffer.Array, off, len);
                    Managers.Network.RaiseQueueStatus(pkt.Position, pkt.Total);
                    break;
                }
            case GateMsgId.SQAdmitted:
                {
                    var pkt = new SQAdmitted();
                    pkt.MergeFrom(buffer.Array, off, len);
                    _admitted = true;
                    Managers.Network.HandleAdmitted(pkt.AuthToken, pkt.GameIp, pkt.GamePort);
                    Disconnect();   // 게이트웨이 종료, 게임 접속은 HandleAdmitted가 처리
                    break;
                }
            default:
                Debug.LogWarning($"[GateClientSession] unknown type: {type}");
                break;
        }
    }

    public override void OnSend(int numOfBytes) { }

    public void SendLeave()
        => Send(PacketUtils.MakeSendBuffer(new CQLeave(), (ushort)GateMsgId.CQLeave));
}