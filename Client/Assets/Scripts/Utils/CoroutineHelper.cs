using System.Collections;
using UnityEngine;

public class CoroutineHelper : MonoBehaviour
{
    static CoroutineHelper _instance;

    public static void Start(IEnumerator coroutine)
    {
        if (_instance == null)
        {
            GameObject go = new GameObject("CoroutineHelper");
            Object.DontDestroyOnLoad(go);
            _instance = go.AddComponent<CoroutineHelper>();
        }
        _instance.StartCoroutine(coroutine);
    }
}
