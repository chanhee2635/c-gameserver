using Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GameScene : BaseScene
{
    UI_GameScene _sceneUI;

    protected override void Init()
    {
        base.Init();
        SceneType = Define.Scene.Game;
        _sceneUI = Managers.UI.ShowSceneUI<UI_GameScene>();
    }

    public void OnEnterGame(ObjectInfo myPlayerInfo)
    {
        Managers.Object.SpawnMyPlayer(myPlayerInfo);

        _sceneUI.SetMyPlayerInfo();

        Managers.Network.ConnectToChatServer();
    }

    public override void Clear()
    {

    }
}
