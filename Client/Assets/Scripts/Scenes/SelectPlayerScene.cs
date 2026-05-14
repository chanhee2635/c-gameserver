using Protocol;
using System.Collections.Generic;
using UnityEngine;

public class SelectPlayerScene : BaseScene
{
    private UI_SelectPlayerScene _sceneUI;

    protected override void Init()
    {
        base.Init();
        SceneType = Define.Scene.SelectPlayer;
        _sceneUI = Managers.UI.ShowSceneUI<UI_SelectPlayerScene>();
    }

    public void SetPlayerSummaries(IEnumerable<PlayerSummary> summaries)
    {
        _sceneUI.SetPlayerSummaries(summaries);
    }

    public void OnCreatePlayerSuccess(PlayerSummary summary)
    {
        _sceneUI.SetCreateResult(summary);
    }

    public void OnCreatePlayerFail(string reason)
    {
        _sceneUI.SetWarningMessage(reason);
    }

    public override void Clear() { }
}
