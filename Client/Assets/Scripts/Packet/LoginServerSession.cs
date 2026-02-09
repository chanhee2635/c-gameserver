using Google.Protobuf;
using Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

public class LoginServerSession : PacketSession
{
    public void Send(IMessage packet)
    {
        string megName = packet.Descriptor.Name.Replace("_", string.Empty);  // SChat
        MsgId msgId = (MsgId)Enum.Parse(typeof(MsgId), megName);  // 지정한 Enum 타입의 값 중에 있는 개체로 변환
        ushort size = (ushort)packet.CalculateSize();  // 패킷 사이즈
        byte[] sendBuffer = new byte[size + 4];  // sendBuffer에 4 더해 만들기(size와 id를 보내기 위해)
        Array.Copy(BitConverter.GetBytes((ushort)(size + 4)), 0, sendBuffer, 0, sizeof(ushort));  // size를 먼저 복사
        Array.Copy(BitConverter.GetBytes((ushort)msgId), 0, sendBuffer, 2, sizeof(ushort));  // id를 복사
        Array.Copy(packet.ToByteArray(), 0, sendBuffer, 4, size);  // 패킷을 바이트배열로 변환하여 복사

        Send(new ArraySegment<byte>(sendBuffer));  // Session 클래스의 Send
    }

    public override void OnConnected(EndPoint endPoint)
    {
        PacketManager.Instance.CustomHandler = (s, m, i) =>
        {
            PacketQueue.Instance.Push(i, m);
        };
    }

    public override void OnDisconnected(EndPoint endPoint)
    {
    }

    public override void OnRecvPacket(ArraySegment<byte> buffer)
    {
        PacketManager.Instance.OnRecvPacket(this, buffer);
    }

    public override void OnSend(int numOfBytes)
    {

    }
}
