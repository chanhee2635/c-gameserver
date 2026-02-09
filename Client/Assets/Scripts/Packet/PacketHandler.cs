using Google.Protobuf;
using Protocol;
using ServerCore;
using UnityEngine;

class PacketHandler
{
    public static void S_LoginAuthHandler(PacketSession session, IMessage message)
    {
        S_LoginAuth packet = message as S_LoginAuth;

        // 로그인 성공 여부에 따라
        if (packet.Success)
        {
            // 토큰을 들고 있는다.
            Managers.Network.Token = packet.AuthToken;

            // 서버 목록 UI호출
            UI_SelectServerPopup popupUI = Managers.UI.ShowPopupUI<UI_SelectServerPopup>();
            popupUI.SetServerList(packet.ServerList);
        }
        else
        {
            UI_LoginScene sceneUI = Managers.UI.SceneUI as UI_LoginScene;
            sceneUI.SetMessage("아이디, 비밀번호를 확인하세요.");
        }
    }

    public static void S_GameConnectedHandler(PacketSession session, IMessage message)
    {
        Debug.Log("S_GameConnected");

        C_AuthToken packet = new C_AuthToken();
        packet.Token = Managers.Network.Token;
        Managers.Network.Send(packet);
    }

    public static void S_PlayerListHandler(PacketSession session, IMessage message)
    {
        S_PlayerList packet = message as S_PlayerList;

        foreach(var info in packet.Summaries)
            Managers.Object.AddPlayerSummary(info);

        Managers.Scene.LoadScene(Define.Scene.SelectPlayer);
    }

    public static void S_EnterGameHandler(PacketSession session, IMessage message)
    {
        S_EnterGame packet = message as S_EnterGame;

        Managers.Object.MyPlayerInfo = packet.MyPlayer;

        // 채팅 서버 접속
        Managers.Network.ConnectToChatServer();

        Managers.Scene.LoadScene(Define.Scene.Game);
    }

    public static void S_SpawnHandler(PacketSession session, IMessage message)
    {
        S_Spawn packet = message as S_Spawn;

        foreach (ObjectInfo info in packet.Infos)
            Managers.Object.AddObject(info);
    }

    public static void S_DespawnHandler(PacketSession session, IMessage message)
    {
        S_Despawn packet = message as S_Despawn;

        foreach (ulong objectId in packet.ObjectIds)
            Managers.Object.HandleDespawn(objectId);
    }

    public static void S_MoveHandler(PacketSession session, IMessage message)
    {
        S_Move packet = message as S_Move;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.SetPosInfo(packet.Pos);
    }

    public static void S_JoinHandler(PacketSession session, IMessage message)
    {
        S_Join packet = message as S_Join;

        // 현재 팝업창에 데이터 전달
        UI_LoginScene sceneUI = Managers.UI.SceneUI as UI_LoginScene;
        if (sceneUI != null && sceneUI.CreateUserPopup != null)
            sceneUI.CreateUserPopup.SetJoinResult(packet.Success);
    }

    public static void S_CreatePlayerHandler(PacketSession session, IMessage message)
    {
        S_CreatePlayer packet = message as S_CreatePlayer;

        UI_SelectPlayerScene sceneUI = Managers.UI.SceneUI as UI_SelectPlayerScene;

        if (sceneUI != null)
        {
            if (packet.Success)
                sceneUI.SetCreateResult(packet.Summary);
            else
                sceneUI.SetWarningMessage("닉네임이 중복되었습니다!");
        }
    }

    public static void S_AttackHandler(PacketSession session, IMessage message)
    {
        S_Attack packet = message as S_Attack;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.OnAttack(packet);
    }

    public static void S_ChangeHpHandler(PacketSession session, IMessage message)
    {
        S_ChangeHp packet = message as S_ChangeHp;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.OnChangeHp(packet);
    }

    public static void S_DieHandler(PacketSession session, IMessage message)
    { 
        S_Die packet = message as S_Die;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.OnDead();

        if (Managers.Object.IsMyPlayer(packet.ObjectId))
        {
            UI_GameScene sceneUI = Managers.UI.SceneUI as UI_GameScene;
            if (sceneUI == null) return;

            sceneUI.ShowRevivePopup();
        }
    }

    public static void S_ChangeStatHandler(PacketSession session, IMessage message)
    {
        S_ChangeStat packet = message as S_ChangeStat;
        Managers.Object.MyPlayer.OnChangeStat(packet.Level);
    }

    public static void S_ChangeExpHandler(PacketSession session, IMessage message)
    {
        S_ChangeExp packet = message as S_ChangeExp;
        Managers.Object.MyPlayer.OnChangeExp(packet.Level, packet.Exp);
    }

    public static void S_ChangeStateHandler(PacketSession session, IMessage message)
    { 
        S_ChangeState packet = message as S_ChangeState;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        CreatureController cc = go.GetComponent<CreatureController>();
        if (cc == null) return;

        cc.SetPosInfo(packet.PosInfo);
    }

    public static void S_ChangeLevelHandler(PacketSession session, IMessage message)
    {
        S_ChangeLevel packet = message as S_ChangeLevel;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        PlayerController pc = go.GetComponent<PlayerController>();
        if (pc == null) return;

        pc.SetLevel(packet.Level);
    }

    public static void S_ChatLoginHandler(PacketSession session, IMessage message)
    {
        S_ChatLogin packet = message as S_ChatLogin;
        Debug.Log("Chat Server Join: " + packet.Success);
    }

    public static void S_ChatHandler(PacketSession session, IMessage message)
    {
        S_Chat packet = message as S_Chat;

        UI_GameScene sceneUI = Managers.UI.SceneUI as UI_GameScene;
        if (sceneUI == null) return;

        sceneUI.RecvChatting(packet);
    }

    public static void S_ReviveHandler(PacketSession session, IMessage message)
    {
        S_Revive packet = message as S_Revive;

        GameObject go = Managers.Object.FindById(packet.ObjectId);
        if (go == null) return;

        PlayerController pc = go.GetComponent<PlayerController>();
        if (pc == null) return;

        pc.SetRevive(packet.PosInfo, packet.Hp);
    }
}
