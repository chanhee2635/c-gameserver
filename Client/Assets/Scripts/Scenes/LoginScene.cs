using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LoginScene : BaseScene
{
    UI_LoginScene _sceneUI;

    protected override void Init()
    {
        base.Init();

        SceneType = Define.Scene.Login;

        _sceneUI = Managers.UI.ShowSceneUI<UI_LoginScene>();

        Managers.Network.ConnectToLoginServer();

        Managers.Object.Clear();
    }

    public override void Clear()
    {
    }
}
