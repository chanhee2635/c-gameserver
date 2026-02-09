using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SelectPlayerScene : BaseScene
{
    UI_SelectPlayerScene _sceneUI;

    protected override void Init()
    {
        base.Init();

        SceneType = Define.Scene.SelectPlayer;

        _sceneUI = Managers.UI.ShowSceneUI<UI_SelectPlayerScene>();
    }

    public override void Clear()
    {
    }
}
