using System;
using System.Collections.Generic;

public sealed partial class PacketManager
{
    partial void Register()
    {
        AddHandler<Protocol.SPlayerList>((ushort)Protocol.MsgId.SPlayerList, PacketHandler.SPlayerListHandler);
        AddHandler<Protocol.SCreatePlayer>((ushort)Protocol.MsgId.SCreatePlayer, PacketHandler.SCreatePlayerHandler);
        AddHandler<Protocol.SEnterGame>((ushort)Protocol.MsgId.SEnterGame, PacketHandler.SEnterGameHandler);
        AddHandler<Protocol.SReadyToEnter>((ushort)Protocol.MsgId.SReadyToEnter, PacketHandler.SReadyToEnterHandler);
        AddHandler<Protocol.SLeaveGame>((ushort)Protocol.MsgId.SLeaveGame, PacketHandler.SLeaveGameHandler);
        AddHandler<Protocol.SUpdateScene>((ushort)Protocol.MsgId.SUpdateScene, PacketHandler.SUpdateSceneHandler);
        AddHandler<Protocol.SAttack>((ushort)Protocol.MsgId.SAttack, PacketHandler.SAttackHandler);
        AddHandler<Protocol.SDie>((ushort)Protocol.MsgId.SDie, PacketHandler.SDieHandler);
        AddHandler<Protocol.SRevive>((ushort)Protocol.MsgId.SRevive, PacketHandler.SReviveHandler);
        AddHandler<Protocol.SChangeHp>((ushort)Protocol.MsgId.SChangeHp, PacketHandler.SChangeHpHandler);
        AddHandler<Protocol.SChangeExp>((ushort)Protocol.MsgId.SChangeExp, PacketHandler.SChangeExpHandler);
        AddHandler<Protocol.SChangeLevel>((ushort)Protocol.MsgId.SChangeLevel, PacketHandler.SChangeLevelHandler);
        AddHandler<Protocol.SChat>((ushort)Protocol.MsgId.SChat, PacketHandler.SChatHandler);
        AddHandler<Protocol.STimeSync>((ushort)Protocol.MsgId.STimeSync, PacketHandler.STimeSyncHandler);
        AddHandler<Protocol.SMoveCorrection>((ushort)Protocol.MsgId.SMoveCorrection, PacketHandler.SMoveCorrectionHandler);
    }
}

public static class PacketIdMapper
{
    static readonly Dictionary<Type, ushort> _map = new()
    {
        { typeof(Protocol.CAuthToken), (ushort)Protocol.MsgId.CAuthToken },
        { typeof(Protocol.CCreatePlayer), (ushort)Protocol.MsgId.CCreatePlayer },
        { typeof(Protocol.CLoadCompleted), (ushort)Protocol.MsgId.CLoadCompleted },
        { typeof(Protocol.CEnterGame), (ushort)Protocol.MsgId.CEnterGame },
        { typeof(Protocol.CLeaveGame), (ushort)Protocol.MsgId.CLeaveGame },
        { typeof(Protocol.CMove), (ushort)Protocol.MsgId.CMove },
        { typeof(Protocol.CAttack), (ushort)Protocol.MsgId.CAttack },
        { typeof(Protocol.CRevive), (ushort)Protocol.MsgId.CRevive },
        { typeof(Protocol.CChat), (ushort)Protocol.MsgId.CChat },
        { typeof(Protocol.CTimeSync), (ushort)Protocol.MsgId.CTimeSync },
    };
    public static ushort GetId(Type t) => _map.TryGetValue(t, out var id) ? id : (ushort)0;
}
