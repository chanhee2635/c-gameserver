using Google.Protobuf;
using ServerCore;
using System;
using System.Net;
using UnityEngine;

public class ServerSession : PacketSession
{
    public override void OnConnected(EndPoint endPoint)
    {
        Debug.Log($"OnConnected : {endPoint}");
        PacketManager.Instance.CustomHandler = pkt => PacketQueue.Instance.Push(pkt);

        Managers.Network.EnqueueMainThread(() =>
        {
            Managers.Scene.LoadScene(Define.Scene.SelectPlayer);
        });

        Send(new Protocol.CAuthToken { AuthToken = Managers.Network.AuthToken });
    }

    public override void OnDisconnected(EndPoint endPoint)
    {
        Debug.Log($"OnDisconnected : {endPoint}");
        Managers.Network.Clear();
        Managers.Network.EnqueueMainThread(() => Managers.Scene.LoadScene(Define.Scene.Login));
    }

    public override void OnRecvPacket(ArraySegment<byte> buffer)
    {
        PacketManager.Instance.OnRecvPacket(buffer);
    }

    public override void OnSend(int numOfBytes) { }


    public void Send(IMessage packet)
    {
        ushort msgId = PacketIdMapper.GetId(packet.GetType());
        if (msgId == 0)
        {
            Debug.LogError($"[ServerSession] 미등록 패킷: {packet.GetType().Name}");
            return;
        }
        Send(PacketUtils.MakeSendBuffer(packet, msgId));
    }
}