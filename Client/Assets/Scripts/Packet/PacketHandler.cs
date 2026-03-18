using Google.Protobuf;
using Protocol;
using ServerCore;
using System.Linq;
using System.Threading;

class PacketHandler
{
    public static void S_LoginAuthHandler(PacketSession session, IMessage message)
    {
        S_LoginAuth packet = message as S_LoginAuth;
        if (packet.Success)
        {
            Managers.Network.Token = packet.AuthToken;
            UI_SelectServerPopup popupUI = Managers.UI.ShowPopupUI<UI_SelectServerPopup>();
            popupUI.SetServerList(packet.ServerList);
        }
        else
        {
            UI_LoginScene sceneUI = Managers.UI.SceneUI as UI_LoginScene;
            sceneUI.SetMessage("아이디, 비밀번호를 확인하세요.");
        }
    }

    public static void S_PlayerListHandler(PacketSession session, IMessage message)
    {
        S_PlayerList packet = message as S_PlayerList;
        var summaries = packet.Players.ToList();

        Managers.Scene.LoadScene(Define.Scene.SelectPlayer, () =>
        {
            var scene = Managers.Scene.CurrentScene as SelectPlayerScene;
            scene?.SetPlayerSummaries(summaries);
        });
    }

    public static void S_CreatePlayerHandler(PacketSession session, IMessage message)
    {
        S_CreatePlayer packet = message as S_CreatePlayer;
        var scene = Managers.Scene.CurrentScene as SelectPlayerScene;
        if (scene == null) return;
        if (packet.Success)
            scene.OnCreatePlayerSuccess(packet.Player);
        else
            scene.OnCreatePlayerFail(packet.Reason);
    }

    public static void S_EnterGameHandler(PacketSession session, IMessage message)
    {
        S_EnterGame packet = message as S_EnterGame;

        if (packet.Success == false)
            return;

        Managers.Scene.LoadScene(Define.Scene.Game, () =>
        {
            var scene = Managers.Scene.CurrentScene as GameScene;
            scene?.OnEnterGame(packet.MyPlayer);
        });
    }

    public static void S_UpdateSceneHandler(PacketSession session, IMessage message)
    {
        S_UpdateScene packet = message as S_UpdateScene;

        foreach (ulong objectId in packet.Despawns)
        {
            if (Managers.Object.IsMyPlayer(objectId)) continue;
            Managers.Object.RemoveObject(objectId);
        }

        foreach (ObjectInfo info in packet.Spawns)
        {
            if (Managers.Object.IsMyPlayer(info.PosInfo.ObjectId)) continue;
            Managers.Object.AddObject(info);
        }

        foreach (PosInfo info in packet.Moves)
        {
            if (Managers.Object.IsMyPlayer(info.ObjectId)) continue;
            CreatureController cc = Managers.Object.FindControllerById(info.ObjectId);
            cc?.OnMoveUpdate(info);
        }
    }


    public static void S_JoinHandler(PacketSession session, IMessage message)
    {
        S_Join packet = message as S_Join;

        // 현재 팝업창에 데이터 전달
        UI_LoginScene sceneUI = Managers.UI.SceneUI as UI_LoginScene;
        if (sceneUI != null && sceneUI.CreateUserPopup != null)
            sceneUI.CreateUserPopup.SetJoinResult(packet.Success);
    }


    public static void S_AttackHandler(PacketSession session, IMessage message)
    {
        S_Attack packet = message as S_Attack;

        if (Managers.Object.IsMyPlayer(packet.ObjectId)) return;

        UnityEngine.GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.OnAttack(packet);
    }

    public static void S_ChangeHpHandler(PacketSession session, IMessage message)
    {
        S_ChangeHp packet = message as S_ChangeHp;

        CreatureController cc = Managers.Object.FindControllerById(packet.ObjectId);
        if (cc == null) return;

        cc.OnChangeHp(packet.Hp, packet.Damage);
    }

    public static void S_DieHandler(PacketSession session, IMessage message)
    { 
        S_Die packet = message as S_Die;

        CreatureController cc = Managers.Object.FindControllerById(packet.ObjectId);
        if (cc == null) return;

        cc.OnDead();

        if (Managers.Object.IsMyPlayer(packet.ObjectId))
        {
            UI_GameScene sceneUI = Managers.UI.SceneUI as UI_GameScene;
            sceneUI?.ShowRevivePopup();
        }
    }

    public static void S_ChangeExpHandler(PacketSession session, IMessage message)
    {
        S_ChangeExp packet = message as S_ChangeExp;

        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;
        if (!Managers.Object.IsMyPlayer(packet.ObjectId)) return;

        player.OnChangeExp(packet.Exp);
    }

    public static void S_ChangeLevelHandler(PacketSession session, IMessage message)
    {
        S_ChangeLevel packet = message as S_ChangeLevel;

        if (Managers.Object.IsMyPlayer(packet.ObjectId))
        {
            Managers.Object.MyPlayer.OnLevelUp(packet.Level, packet.MaxHp, packet.Hp, packet.Exp);
        }
        else
        {
            CreatureController cc = Managers.Object.FindControllerById(packet.ObjectId);
            cc?.OnLevelUp(packet.Level, packet.MaxHp, packet.Hp);
        }
    }
    public static void S_ReviveHandler(PacketSession session, IMessage message)
    {
        S_Revive packet = message as S_Revive;

        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;

        UnityEngine.Vector3 revivePos = new UnityEngine.Vector3(packet.Pos.X, packet.Pos.Y, packet.Pos.Z);
        player.OnRevive(revivePos, packet.Hp, packet.MaxHp);
    }

    public static void S_ChatLoginHandler(PacketSession session, IMessage message)
    {
        Managers.Network.Send(new C_LoadCompleted());
    }

    public static void S_ChatHandler(PacketSession session, IMessage message)
    {
        S_Chat packet = message as S_Chat;

        UI_GameScene sceneUI = Managers.UI.SceneUI as UI_GameScene;
        if (sceneUI == null) return;

        sceneUI.RecvChatting(packet);
    }
}
