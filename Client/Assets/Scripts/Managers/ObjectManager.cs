using System.Collections.Generic;
using UnityEngine;

public class ObjectManager
{
    // 시야 변수
    float _checkInterval = 0.2f;
    float _lastCheckTime = 0f;
    float _viewDistance = 30.0f;

    public MyPlayerController MyPlayer { get; private set; }
    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();
    Dictionary<ulong, CreatureController> _controllers = new Dictionary<ulong, CreatureController>();
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

    public void SpawnMyPlayer(Protocol.ObjectInfo info)
    {
        PrefabData data = Managers.Data.PrefabDataDict[info.Summary.TemplateId];
        Vector3 position = GetSpawnPosition(info.PosInfo);
        Quaternion rotation = Quaternion.Euler(0, info.PosInfo.Yaw, 0);

        GameObject go = Managers.Resource.Instantiate(data.myPrefabPath, position, rotation);
        go.SetActive(true);

        MyPlayerController mpc = go.GetOrAddComponent<MyPlayerController>();
        mpc.SetInfo(info, position, rotation);
        MyPlayer = mpc;

        _objects[info.Summary.ObjectId] = go;
        _controllers[info.Summary.ObjectId] = mpc;
    }


    public void AddObject(Protocol.ObjectInfo info)
    {
        if (_objects.ContainsKey(info.Summary.ObjectId)) return;

        PrefabData data = Managers.Data.PrefabDataDict[info.Summary.TemplateId];
        Vector3 position = GetSpawnPosition(info.PosInfo);
        Quaternion rotation = Quaternion.Euler(0, info.PosInfo.Yaw, 0);
        
        GameObject go = Managers.Resource.Instantiate(data.prefabPath, position, rotation);
        _objects[info.Summary.ObjectId] = go;

        BaseController controller = null;
        switch (info.Summary.ObjectType) {
            case Protocol.GameObjectType.Player: controller = go.GetOrAddComponent<PlayerController>(); break;
            case Protocol.GameObjectType.Monster: controller = go.GetOrAddComponent<MonsterController>(); break;
        }
        if (controller != null)
        {
            controller.SetInfo(info, position, rotation);
            if (controller is CreatureController cc)
                _controllers[info.Summary.ObjectId] = cc;
        }
        if (MyPlayer != null)
        {
            float distSq = (MyPlayer.transform.position - go.transform.position).sqrMagnitude;
            go.SetActive(distSq <= _viewDistance * _viewDistance);
        }
    }

    public void RemoveObject(ulong objectId)
    {
        if (MyPlayer != null && MyPlayer.GetObjectId() == objectId) return;

        if (_objects.TryGetValue(objectId, out GameObject obj))
        {
            _objects.Remove(objectId);
            _controllers.Remove(objectId);
            Managers.Resource.Destroy(obj);
        }
    }
    public GameObject FindById(ulong id)
    {
        _objects.TryGetValue(id, out GameObject go);
        return go;
    }

    public CreatureController FindControllerById(ulong id)
    {
        _controllers.TryGetValue(id, out CreatureController cc);
        return cc;
    }

    public bool IsMyPlayer(ulong id) => MyPlayer != null && MyPlayer.GetObjectId() == id;

    public void Clear()
    {
        MyPlayer = null;
        foreach (GameObject obj in _objects.Values)
            Managers.Resource.Destroy(obj);
        _objects.Clear();
        _controllers.Clear();
    }

    Vector3 GetSpawnPosition(Protocol.PosInfo posInfo)
    {
        Vector3 pos = new Vector3(posInfo.Pos.X, posInfo.Pos.Y, posInfo.Pos.Z);
        if (Physics.Raycast(pos + Vector3.up * 20f, Vector3.down, out RaycastHit hit, 40f, LayerMask.GetMask("Ground")))
            pos.y = hit.point.y;
        return pos;
    }
}
