using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneManagerEx
{
    public BaseScene CurrentScene { get { return GameObject.FindObjectOfType<BaseScene>(); } }

    public void LoadScene(Define.Scene type, Action onLoaded = null)
    {
        Managers.Clear();
        Managers.Instance.StartCoroutine(LoadSceneAsync(type, onLoaded));
    }

    string GetSceneName(Define.Scene type)
    {
        return System.Enum.GetName(typeof(Define.Scene), type);
    }

    public void Clear()
    {
        CurrentScene.Clear();
    }

    IEnumerator LoadSceneAsync(Define.Scene type, Action onLoaded)
    {
        AsyncOperation op = SceneManager.LoadSceneAsync(GetSceneName(type));
        yield return op;
        onLoaded?.Invoke();
    }
}
