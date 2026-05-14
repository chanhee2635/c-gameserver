using System.Collections.Generic;
using UnityEngine;

public class ObjectManager
{
    private const float CHECK_INTERVAL = 0.2f;
    private const float SHOW_DIST_SQ   = 600.0f;
    private const float HIDE_DIST_SQ   = 660.0f;

    private float _lastCheckTime;

    public Protocol.ObjectInfo    MyPlayerInfo { get; private set; }
    public MyPlayerController     MyPlayer     { get; private set; }

    private Dictionary<ulong, Protocol.ObjectInfo> _pendingSpawns = new Dictionary<ulong, Protocol.ObjectInfo>();

    private Dictionary<ulong, GameObject>          _objects         = new Dictionary<ulong, GameObject>();
    private Dictionary<ulong, CreatureController>  _controllers     = new Dictionary<ulong, CreatureController>();
    private List<CreatureController>               _cachedCreatures = new List<CreatureController>(2000);
    private Dictionary<ulong, int>                 _creatureIndex   = new();

    public void Update()
    {
        if (MyPlayer == null || !IsGameSceneReady()) return;
        if (Time.time < _lastCheckTime + CHECK_INTERVAL) return;
        _lastCheckTime = Time.time;

        Vector3 myPos = MyPlayer.transform.position;

        MyPlayer.UpdateUIByDistance(0f);

        for (int i = 0; i < _cachedCreatures.Count; ++i)
        {
            CreatureController cc = _cachedCreatures[i];
            if (cc == null) continue;

            UpdateObjectVisibility(cc, myPos);
        }
    }

    private void UpdateObjectVisibility(CreatureController cc, Vector3 myPos)
    {
        GameObject go    = cc.gameObject;
        float distSq     = (myPos - go.transform.position).sqrMagnitude;
        bool  isActive   = go.activeSelf;

        var sceneUI = Managers.UI.SceneUI as UI_GameScene;
        if (isActive)
        {
            if (distSq > HIDE_DIST_SQ)
            {
                go.SetActive(false);
                sceneUI?.Minimap?.ShowIcon(cc.ObjectId, false);
            }
            else
            {
                cc.UpdateUIByDistance(distSq);
            }
        }
        else
        {
            if (distSq <= SHOW_DIST_SQ)
            {
                go.SetActive(true);
                cc.UpdateUIByDistance(distSq);
                sceneUI?.Minimap?.ShowIcon(cc.ObjectId, true);
            }
        }
    }

    public void SetMyPlayerInfo(Protocol.ObjectInfo info) => MyPlayerInfo = info;

    public void SpawnMyPlayer()
    {
        if (MyPlayerInfo == null) return;

        int templateId       = MyPlayerInfo.Summary.TemplateId;
        PrefabData data      = Managers.Data.PrefabDataDict[templateId];

        Vector3    pos = GetSpawnPosition(MyPlayerInfo.PosInfo);
        Quaternion rot = Quaternion.Euler(0, MyPlayerInfo.PosInfo.Yaw, 0);

        GameObject go = Managers.Resource.Instantiate(data.myPrefabPath, pos, rot);
        if (go == null) return;

        go.SetActive(true);

        MyPlayerController mpc = go.GetOrAddComponent<MyPlayerController>();
        mpc.SetInfo(MyPlayerInfo, pos, rot);
        MyPlayer = mpc;

        ulong objectId       = MyPlayerInfo.Summary.ObjectId;
        _objects[objectId]     = go;
        _controllers[objectId] = mpc;
    }

    public void AddPendingObject(Protocol.ObjectInfo info)
    {
        if (IsMyPlayer(info.Summary.ObjectId)) return;

        if (IsGameSceneReady())
            Spawn(info);
        else
            _pendingSpawns[info.Summary.ObjectId] = info;
    }

    public void Despawn(ulong objectId)
    {
        if (IsMyPlayer(objectId)) return;

        _pendingSpawns.Remove(objectId);

        if (_controllers.Remove(objectId, out CreatureController cc))
        {
            if (_creatureIndex.TryGetValue(objectId, out int idx))
            {
                int last = _cachedCreatures.Count - 1;
                if (idx != last)
                {
                    _cachedCreatures[idx] = _cachedCreatures[last];
                    _creatureIndex[_cachedCreatures[idx].ObjectId] = idx;
                }
                _cachedCreatures.RemoveAt(last);  // O(1)
                _creatureIndex.Remove(objectId);
            }
        }

        if (_objects.Remove(objectId, out GameObject go))
        {
            (Managers.UI.SceneUI as UI_GameScene)?.Minimap?.RemoveIcon(objectId);
            Managers.Resource.Destroy(go);
        }
    }

    private void Spawn(Protocol.ObjectInfo info)
    {
        ulong objectId = info.Summary.ObjectId;
        if (_objects.ContainsKey(objectId)) return;

        PrefabData data = Managers.Data.PrefabDataDict[info.Summary.TemplateId];
        Vector3    pos  = GetSpawnPosition(info.PosInfo);
        Quaternion rot  = Quaternion.Euler(0, info.PosInfo.Yaw, 0);

        GameObject go = Managers.Resource.Instantiate(data.prefabPath, pos, rot);
        _objects[objectId] = go;

        CreatureController cc = info.Summary.ObjectType switch
        {
            Protocol.GameObjectType.Player  => go.GetOrAddComponent<PlayerController>(),
            Protocol.GameObjectType.Monster => go.GetOrAddComponent<MonsterController>(),
            _ => null
        };

        if (cc != null)
        {
            cc.SetInfo(info, pos, rot);
            _controllers[objectId] = cc;
            _cachedCreatures.Add(cc);
            _creatureIndex[objectId] = _cachedCreatures.Count - 1;

            if (MyPlayer != null)
                UpdateObjectVisibility(cc, MyPlayer.transform.position);
            else
                go.SetActive(false);
        }
    }

    public void SpawnPendingObjects()
    {
        foreach (var info in _pendingSpawns.Values)
            Spawn(info);
        _pendingSpawns.Clear();
    }

    public void OnMove(Protocol.PosInfo movePos)
    {
        if (IsMyPlayer(movePos.ObjectId)) return;

        if (_controllers.TryGetValue(movePos.ObjectId, out CreatureController cc))
            cc.OnMoveUpdate(movePos);
    }

    public GameObject          FindById(ulong id)           => _objects.GetValueOrDefault(id);
    public CreatureController  FindControllerById(ulong id) => _controllers.GetValueOrDefault(id);
    public bool                IsMyPlayer(ulong id)         => MyPlayerInfo?.Summary.ObjectId == id;
    public List<CreatureController> GetCachedCreatures()    => _cachedCreatures;

    private bool    IsGameSceneReady()                    => Managers.Scene.CurrentScene is GameScene { IsSceneReady: true };
    private Vector3 GetSpawnPosition(Protocol.PosInfo posInfo)
    {
        Vector3 pos = new Vector3(posInfo.Pos.X, posInfo.Pos.Y, posInfo.Pos.Z);
        if (Physics.Raycast(pos + Vector3.up * 20f, Vector3.down, out RaycastHit hit, 40f, LayerMask.GetMask("Ground")))
            pos.y = hit.point.y;
        return pos;
    }

    public void Clear()
    {
        foreach (GameObject obj in _objects.Values)
            Managers.Resource.Destroy(obj);
        _objects.Clear();
        _controllers.Clear();
        _cachedCreatures.Clear();
        _pendingSpawns.Clear();
        _creatureIndex.Clear();
        MyPlayer     = null;
        MyPlayerInfo = null;
    }
}
