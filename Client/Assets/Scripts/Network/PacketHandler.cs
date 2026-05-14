using UnityEngine;

public static partial class PacketHandler
{
    public static void SPlayerListHandler(Protocol.SPlayerList pkt)
    {
        if (!pkt.Success) return;

        Managers.Scene.ActivateScene(() =>
        {
            var scene = Managers.Scene.CurrentScene as SelectPlayerScene;
            scene?.SetPlayerSummaries(pkt.Players);
        });
    }

    public static void SCreatePlayerHandler(Protocol.SCreatePlayer pkt)
    {
        var scene = Managers.Scene.CurrentScene as SelectPlayerScene;
        if (scene == null) return;

        if (pkt.Success)
            scene.OnCreatePlayerSuccess(pkt.Player);
        else
            scene.OnCreatePlayerFail(pkt.Reason);
    }

    public static void SEnterGameHandler(Protocol.SEnterGame pkt)
    {
        if (!pkt.Success) return;
        Managers.Object.SetMyPlayerInfo(pkt.MyPlayer);
    }

    public static void SReadyToEnterHandler(Protocol.SReadyToEnter pkt)
    {
        Managers.Scene.ActivateScene();
    }

    public static void SLeaveGameHandler(Protocol.SLeaveGame pkt) { }

    public static void SAttackHandler(Protocol.SAttack pkt)
    {
        if (Managers.Object.IsMyPlayer(pkt.ObjectId)) return;
        Managers.Object.FindControllerById(pkt.ObjectId)?.OnAttack(pkt);
    }

    public static void SDieHandler(Protocol.SDie pkt)
    {
        Managers.Object.FindControllerById(pkt.ObjectId)?.OnDead();

        if (Managers.Object.IsMyPlayer(pkt.ObjectId))
        {
            var sceneUI = Managers.UI.SceneUI as UI_GameScene;
            sceneUI?.ShowRevivePopup();
        }
    }

    public static void SReviveHandler(Protocol.SRevive pkt)
    {
        Vector3 pos = new Vector3(pkt.Pos.X, pkt.Pos.Y, pkt.Pos.Z);
        Managers.Object.MyPlayer?.OnRevive(pos, pkt.Hp, pkt.MaxHp);

        var sceneUI = Managers.UI.SceneUI as UI_GameScene;
        sceneUI?.HideReviveUI();
    }

    public static void SChangeHpHandler(Protocol.SChangeHp pkt)
    {
        Managers.Object.FindControllerById(pkt.ObjectId)?.OnChangeHp(pkt.Hp, pkt.Damage);
    }

    public static void SChangeExpHandler(Protocol.SChangeExp pkt)
    {
        if (!Managers.Object.IsMyPlayer(pkt.ObjectId)) return;
        Managers.Object.MyPlayer?.OnChangeExp(pkt.Exp);
    }

    public static void SChangeLevelHandler(Protocol.SChangeLevel pkt)
    {
        if (Managers.Object.IsMyPlayer(pkt.ObjectId))
        {
            Managers.Object.MyPlayer?.OnLevelUp(pkt.Level, pkt.MaxHp, pkt.Hp, pkt.Exp);
        }
        else
        {
            CreatureController cc = Managers.Object.FindControllerById(pkt.ObjectId);
            if (cc is PlayerController pc)
                pc.OnLevelUp(pkt.Level, pkt.MaxHp, pkt.Hp);
        }
    }

    public static void SUpdateSceneHandler(Protocol.SUpdateScene pkt)
    {
        foreach (var obj in pkt.Spawns)
        {
            Managers.Object.AddPendingObject(obj);
        }

        foreach (uint objectId in pkt.Despawns)
        {
            Managers.Object.Despawn(objectId);
        }

        foreach (var movePos in pkt.Moves)
        {
            if (Managers.Scene.CurrentScene is GameScene { IsSceneReady: true })
            {
                Managers.Object.OnMove(movePos);
            }
        }
    }

    public static void SChatHandler(Protocol.SChat pkt) { }
}
