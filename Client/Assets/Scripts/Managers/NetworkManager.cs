using Google.Protobuf;
using Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Net;

public class NetworkManager
{
    public string Token { get; set; }

    LoginServerSession _loginServerSession = new LoginServerSession();
    ChatServerSession _chatServerSession = new ChatServerSession();
    ServerSession _gameServerSession = new ServerSession();

    public void Send(IMessage packet)
    {
        _gameServerSession.Send(packet);
    }

    public void SendToLogin(IMessage packet)
    {
        _loginServerSession.Send(packet);
    }

    public void SendToChat(IMessage packet)
    {
        _chatServerSession.Send(packet);
    }

    public void ConnectToLoginServer()
    {
        IPAddress ipAddr = IPAddress.Parse("127.0.0.1");
        IPEndPoint endPoint = new IPEndPoint(ipAddr, 7778);

        Connector connector = new Connector();
        connector.Connect(endPoint, () => { return _loginServerSession; }, 1);
    }

    public void ConnectToChatServer()
    {
        IPAddress ipAddr = IPAddress.Parse("127.0.0.1");
        IPEndPoint endPoint = new IPEndPoint(ipAddr, 7779);

        Connector connector = new Connector();
        connector.Connect(endPoint, () => { return _chatServerSession; }, 1);
    }

    public void DisconnectToLoginServer()
    {
        _loginServerSession.Disconnect();
    }

    public void DisconnectToChatServer()
    {
        _chatServerSession.Disconnect();
    }

    public void ConnectToGame(ServerInfo info)
    {
        IPAddress ipAddr = IPAddress.Parse(info.Ip);
        IPEndPoint endPoint = new IPEndPoint(ipAddr, (int)info.Port);

        Connector connector = new Connector();
        connector.Connect(endPoint, () => {
            _gameServerSession = new ServerSession();
            return _gameServerSession; 
        }, 1);
    }

    public void Update()
    {
        List<PacketMessage> list = PacketQueue.Instance.PopAll();
        foreach (PacketMessage packet in list)
        {
            Action<PacketSession, IMessage> handler = PacketManager.Instance.GetPacketHandler(packet.Id);
            if (handler != null)
            {
                if (packet.Id < 1000)
                    continue;
                else if (packet.Id < 2000)
                    handler.Invoke(_loginServerSession, packet.Message);
                else if (packet.Id < 3000)
                    handler.Invoke(_gameServerSession, packet.Message);
                else if (packet.Id < 4000)
                    handler.Invoke(_chatServerSession, packet.Message);
            }
        }
    }
}
