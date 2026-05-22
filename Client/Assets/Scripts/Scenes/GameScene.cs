using Protocol;
using UnityEngine;

public class GameScene : BaseScene
{
    public bool IsSceneReady { get; private set; }

    private UI_GameScene _sceneUI;

    protected override void Init()
    {
        base.Init();
        SceneType = Define.Scene.Game;
        _sceneUI = Managers.UI.ShowSceneUI<UI_GameScene>();
        CreatePools();

        Managers.Object.SpawnMyPlayer();

        IsSceneReady = true;

        Managers.Object.SpawnPendingObjects();
    }

    private void CreatePools()
    {
        foreach (var pair in Managers.Data.PrefabDataDict)
        {
            PrefabData data = pair.Value;

            CreatePoolIfValid(data.prefabPath, data.poolSize);
            CreatePoolIfValid(data.dummyPrefabPath, data.poolSize);
        }
    }

    private void CreatePoolIfValid(string path, int poolSize)
    {
        if (string.IsNullOrEmpty(path)) return;
        var prefab = Managers.Resource.Load<GameObject>($"Prefabs/{path}");
        if (prefab != null && prefab.GetComponent<Poolable>() != null)
            Managers.Pool.CreatePool(prefab, poolSize);
    }

    public override void Clear() => IsSceneReady = false;
}
