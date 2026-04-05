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

        CreatePools();
    }

    void CreatePools()
    {
        foreach(var pair in Managers.Data.PrefabDataDict)
        {
            PrefabData data = pair.Value;

            if (!string.IsNullOrEmpty(data.prefabPath))
            {
                GameObject prefab = Managers.Resource.Load<GameObject>($"Prefabs/{data.prefabPath}");
                if (prefab != null && prefab.GetComponent<Poolable>() != null)
                    Managers.Pool.CreatePool(prefab, data.poolSize);
            }
        }
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
