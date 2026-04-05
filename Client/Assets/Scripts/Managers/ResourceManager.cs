using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ResourceManager
{
    public T Load<T>(string path) where T : Object
    {
        return Resources.Load<T>(path);
    }

    public GameObject Instantiate(string path, Transform parent = null)
    {
        GameObject prefab = Load<GameObject>($"Prefabs/{path}");
        if (prefab == null) return null;

        if (prefab.GetComponent<Poolable>() != null)
        {
            GameObject go = Managers.Pool.Pop(prefab, parent).gameObject;
            go.SetActive(true);
            return go;
        }

        return Object.Instantiate(prefab, parent);
    }

    public GameObject Instantiate(string path, Vector3 position, Quaternion rotation)
    {
        GameObject prefab = Load<GameObject>($"Prefabs/{path}");
        if (prefab == null) return null;

        if (prefab.GetComponent<Poolable>() != null)
        {
            GameObject go = Managers.Pool.Pop(prefab).gameObject;
            go.transform.SetPositionAndRotation(position, rotation);
            go.SetActive(true);
            return go;
        }

        return Object.Instantiate(prefab, position, rotation);
    }

    public void Destroy(GameObject go)
    {
        if (go == null)
            return;

        Poolable poolable = go.GetComponent<Poolable>();
        if (poolable != null)
        {
            Managers.Pool.Push(poolable);
            return;
        }

        Object.Destroy(go);
    }
}
