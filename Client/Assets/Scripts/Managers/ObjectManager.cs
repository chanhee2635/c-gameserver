using Protocol;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class ObjectManager
{
    public bool IsSceneActive { get; private set; } = false;
    public ObjectInfo MyPlayerInfo { get; set; }
    public MyPlayerController MyPlayer { get; private set; }
    public List<SummaryInfo> Summaries { get; private set; } = new List<SummaryInfo>();

    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();

    float _checkInterval = 0.2f;
    float _lastCheckTime = 0f;
    float _viewDistance = 30.0f;

    public void Update()
    {
        if (MyPlayer == null) return;

        if (Time.time < _lastCheckTime + _checkInterval) return;
        _lastCheckTime = Time.time;

        foreach (var pair in _objects)
        {
            GameObject go = pair.Value;
            if (MyPlayer.gameObject == go) continue;

            float distSq = (MyPlayer.transform.position - go.transform.position).sqrMagnitude;
            bool shouldActive = distSq <= (_viewDistance * _viewDistance);

            if (go.activeSelf != shouldActive)
                go.SetActive(shouldActive);
        }
    }

    public GameObject FindById(ulong id)
    {
        GameObject go = null;
        if (_objects.TryGetValue(id, out go))
            return go;
        return null;
    }

    public bool IsMyPlayer(ulong id)
    {
        if (MyPlayer == null) return false;
        return MyPlayer.ObjectId == id;
    }

    public void HandleDespawn(ulong objectId)
    {
        RemoveObject(objectId);
    }

    public void MyPlayerSpawn()
    {
        AddObject(MyPlayerInfo);
    }

    public void AddObject(ObjectInfo info)
    {
        if (_objects.TryGetValue(info.Summary.ObjectId, out GameObject go))
            return;

        PrefabData data = Managers.Data.PrefabDataDict[info.Summary.TemplateId];
        go = Managers.Resource.Instantiate(data.prefabPath);
        if (MyPlayer != null && go != MyPlayer.gameObject)
        {
            float distSq = (MyPlayer.transform.position - go.transform.position).sqrMagnitude;
            go.SetActive(distSq <= _viewDistance * _viewDistance);
        }
        _objects.TryAdd(info.Summary.ObjectId, go);

        switch (info.Summary.ObjectType)
        {
            case GameObjectType.Player:
            {
                if (MyPlayerInfo.Summary.ObjectId == info.Summary.ObjectId)
                {
                    go.SetActive(true);
                    MyPlayerController mpc = go.GetOrAddComponent<MyPlayerController>();
                    mpc.SetInfo(info);
                    MyPlayer = mpc;
                }
                else
                {
                    PlayerController pc = go.GetOrAddComponent<PlayerController>();
                    pc.SetInfo(info);
                }
                break;
            }
            case GameObjectType.Monster:
            {
                MonsterController mc = go.GetOrAddComponent<MonsterController>();
                mc.SetInfo(info);
                break;
            }
        }
    }

    private void RemoveObject(ulong objectId)
    {
        if (_objects.TryGetValue(objectId, out GameObject obj))
        {
            _objects.Remove(objectId);
            Managers.Resource.Destroy(obj);
        }
    }

    public void Clear()
    {
        MyPlayerInfo = null;
        MyPlayer = null;
        Summaries.Clear();
        foreach (GameObject obj in _objects.Values)
            Managers.Resource.Destroy(obj);

        _objects.Clear();
    }

    public void AddPlayerSummary(SummaryInfo summary)
    {
        Summaries.Add(summary);
    }
}
