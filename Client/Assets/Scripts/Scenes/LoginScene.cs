using UnityEngine;

public class LoginScene : BaseScene
{
    protected override void Init()
    {
        base.Init();
        SceneType = Define.Scene.Login;
        Managers.UI.ShowSceneUI<UI_LoginScene>();
        Managers.Object.Clear();
    }

    public override void Clear() { }
}
