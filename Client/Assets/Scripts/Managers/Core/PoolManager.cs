using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class PoolManager
{
    #region Pool
    class Pool
    {
        public GameObject Original { get; private set; }  // 원본 저장 (새로 로드X)
        public Transform  Root     { get; private set; }

        private Stack<Poolable> _poolStack = new Stack<Poolable>();

        public void Init(GameObject original, int count = 5)
        {
            Original   = original;
            Root       = new GameObject().transform;
            Root.name  = $"{original.name}_Root";

            for (int i = 0; i < count; i++)
                Push(Create());
        }

        // Original 객체 복사 및 반환
        private Poolable Create()
        {
            GameObject go = Object.Instantiate<GameObject>(Original);
            go.name = Original.name;
            return go.GetOrAddComponent<Poolable>();
        }

        // Pool 에 반납
        public void Push(Poolable poolable)
        {
            if (poolable == null)
                return;

            poolable.transform.SetParent(Root, false);
            poolable.gameObject.SetActive(false);
            poolable.IsUsing = false;

            _poolStack.Push(poolable);
        }

        public Poolable Pop(Transform parent)
        {
            Poolable poolable = _poolStack.Count > 0 ? _poolStack.Pop() : Create();

            if (parent == null)
            {
                BaseScene scene = Managers.Scene.CurrentScene;
                poolable.transform.SetParent(scene != null ? scene.transform : null);
            }
            else
            {
                // worldPositionStays=false: RectTransform 레이아웃을 보존한다.
                // true(기본값)로 두면 UI 요소의 크기·위치가 월드 좌표 기준으로 재계산되어
                // Canvas 내부 아이템이 엉뚱한 위치에 배치된다.
                poolable.transform.SetParent(parent, false);
            }

            poolable.IsUsing = true;
            return poolable;
        }
    }
    #endregion

    private Dictionary<string, Pool> _pool = new Dictionary<string, Pool>();
    private Transform _root;

    public void Init()
    {
        if (_root == null)
        {
            _root = new GameObject { name = "@Pool_Root" }.transform;
            Object.DontDestroyOnLoad(_root);
        }
    }

    public void CreatePool(GameObject original, int count = 5)
    {
        if (_pool.ContainsKey(original.name)) return;
        Pool pool = new Pool();
        pool.Init(original, count);
        pool.Root.parent = _root;
        _pool.Add(original.name, pool);
    }

    public void Push(Poolable poolable)
    {
        string name = poolable.gameObject.name;
        if (!_pool.TryGetValue(name, out Pool pool))
        {
            GameObject.Destroy(poolable.gameObject);
            return;
        }
        pool.Push(poolable);
    }

    public Poolable Pop(GameObject original, Transform parent = null)
    {
        if (!_pool.ContainsKey(original.name))
            CreatePool(original);

        return _pool[original.name].Pop(parent);
    }

    public GameObject GetOriginal(string name)
    {
        return _pool.TryGetValue(name, out Pool pool) ? pool.Original : null;
    }

    public void Clear()
    {
        foreach (Transform child in _root)
            GameObject.Destroy(child.gameObject);

        _pool.Clear();
    }
}
