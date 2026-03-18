using Google.Protobuf;
using Protocol;
using ServerCore;
using System;
using System.Collections.Generic;

class PacketManager
{
    #region Singleton
    static PacketManager _instance = new PacketManager();
    public static PacketManager Instance { get { return _instance; } }
    #endregion

    PacketManager()
    {
        Register();
    }

    Dictionary<ushort, Action<PacketSession, ArraySegment<byte>, ushort>> _onRecv = new Dictionary<ushort, Action<PacketSession, ArraySegment<byte>, ushort>>();
    Dictionary<ushort, Action<PacketSession, IMessage>> _handler = new Dictionary<ushort, Action<PacketSession, IMessage>>();

    public Action<PacketSession, IMessage, ushort> CustomHandler { get; set; }

    public void Register()
    {
        _onRecv.Add((ushort)MsgId.SLoginAuth, MakePacket <S_LoginAuth>);
        _handler.Add((ushort)MsgId.SLoginAuth, PacketHandler.S_LoginAuthHandler);
        _onRecv.Add((ushort)MsgId.SJoin, MakePacket <S_Join>);
        _handler.Add((ushort)MsgId.SJoin, PacketHandler.S_JoinHandler);
        _onRecv.Add((ushort)MsgId.SPlayerList, MakePacket <S_PlayerList>);
        _handler.Add((ushort)MsgId.SPlayerList, PacketHandler.S_PlayerListHandler);
        _onRecv.Add((ushort)MsgId.SCreatePlayer, MakePacket <S_CreatePlayer>);
        _handler.Add((ushort)MsgId.SCreatePlayer, PacketHandler.S_CreatePlayerHandler);
        _onRecv.Add((ushort)MsgId.SEnterGame, MakePacket <S_EnterGame>);
        _handler.Add((ushort)MsgId.SEnterGame, PacketHandler.S_EnterGameHandler);
        _onRecv.Add((ushort)MsgId.SAttack, MakePacket <S_Attack>);
        _handler.Add((ushort)MsgId.SAttack, PacketHandler.S_AttackHandler);
        _onRecv.Add((ushort)MsgId.SDie, MakePacket <S_Die>);
        _handler.Add((ushort)MsgId.SDie, PacketHandler.S_DieHandler);
        _onRecv.Add((ushort)MsgId.SChangeHp, MakePacket <S_ChangeHp>);
        _handler.Add((ushort)MsgId.SChangeHp, PacketHandler.S_ChangeHpHandler);
        _onRecv.Add((ushort)MsgId.SChangeExp, MakePacket <S_ChangeExp>);
        _handler.Add((ushort)MsgId.SChangeExp, PacketHandler.S_ChangeExpHandler);
        _onRecv.Add((ushort)MsgId.SChangeLevel, MakePacket <S_ChangeLevel>);
        _handler.Add((ushort)MsgId.SChangeLevel, PacketHandler.S_ChangeLevelHandler);
        _onRecv.Add((ushort)MsgId.SUpdateScene, MakePacket <S_UpdateScene>);
        _handler.Add((ushort)MsgId.SUpdateScene, PacketHandler.S_UpdateSceneHandler);
        _onRecv.Add((ushort)MsgId.SRevive, MakePacket <S_Revive>);
        _handler.Add((ushort)MsgId.SRevive, PacketHandler.S_ReviveHandler);
        _onRecv.Add((ushort)MsgId.SChatLogin, MakePacket <S_ChatLogin>);
        _handler.Add((ushort)MsgId.SChatLogin, PacketHandler.S_ChatLoginHandler);
        _onRecv.Add((ushort)MsgId.SChat, MakePacket <S_Chat>);
        _handler.Add((ushort)MsgId.SChat, PacketHandler.S_ChatHandler);
        
    }

    /// <summary>
    /// 수신된 바이트 배열을 패킷 객체로 변환하고 처리 함수(Handler)를 호출
    /// </summary>
    /// <param name="session">패킷을 송신한 세션 객체</param>
    /// <param name="buffer">패킷 헤더를 포함한 전체 데이터 버퍼</param>
    public void OnRecvPacket(PacketSession session, ArraySegment<byte> buffer)
    {
        ushort count = 0;
        ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset);
        count += 2;
        ushort id = BitConverter.ToUInt16(buffer.Array, buffer.Offset + count);
        count += 2;

        if (_onRecv.TryGetValue(id, out var action))
            action.Invoke(session, buffer, id);
    }

    /// <summary>
    /// Google Protobuf를 사용하여 패킷 객체를 생성하고 처리 함수(Handler)로 전달
    /// </summary>
    /// <typeparam name="T">IMessage를 상속받은 프로토콜 버퍼 객체 타입</typeparam>
    /// <param name="session">패킷을 송신한 세션 객체</param>
    /// <param name="buffer">전체 데이터 버퍼</param>
    /// <param name="id">패킷 Id</param>
    void MakePacket<T>(PacketSession session, ArraySegment<byte> buffer, ushort id) where T : IMessage<T>, new()
    {
        T pkt = new();
        // 헤더를 제외한 나머지 데이터 역직렬화
        pkt.MergeFrom(buffer.Array, buffer.Offset + 4, buffer.Count - 4);

        if (CustomHandler != null)
            CustomHandler.Invoke(session, pkt, id);
        else if (_handler.TryGetValue(id, out var action))
            action.Invoke(session, pkt);
    }

    public Action<PacketSession, IMessage> GetPacketHandler(ushort id)
    {
        if (_handler.TryGetValue(id, out var action))
            return action;
        return null;
    }
}