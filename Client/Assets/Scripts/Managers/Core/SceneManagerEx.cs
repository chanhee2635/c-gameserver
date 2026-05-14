using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneManagerEx
{
    public BaseScene CurrentScene { get; private set; }

    private AsyncOperation _pendingSceneOp;

    public void LoadScene(Define.Scene type, Action onLoaded = null)
    {
        Managers.Instance.ClearScene();
        Managers.Instance.StartCoroutine(LoadSceneAsync(type, onLoaded));
    }

    private IEnumerator LoadSceneAsync(Define.Scene type, Action onLoaded)
    {
        _pendingSceneOp = SceneManager.LoadSceneAsync(GetSceneName(type));

        _pendingSceneOp.allowSceneActivation = false;

        while (_pendingSceneOp.progress < 0.9f)
        {
            yield return null;
        }

        onLoaded?.Invoke();
    }

    public void ActivateScene(Action onActive = null)
    {
        if (_pendingSceneOp != null)
        {
            _pendingSceneOp.allowSceneActivation = true;
            Managers.Instance.StartCoroutine(CoWaitAndReport(onActive));
        }
    }

    public void RegisterCurrentScene(BaseScene scene)
    {
        CurrentScene = scene;
    }

    private IEnumerator CoWaitAndReport(Action onActive)
    {
        yield return new WaitUntil(() => _pendingSceneOp.isDone);

        onActive?.Invoke();

        Managers.Network.Send(new Protocol.CLoadCompleted());
        _pendingSceneOp = null;
    }

    private string GetSceneName(Define.Scene type)
    {
        return System.Enum.GetName(typeof(Define.Scene), type);
    }

    public void Clear()
    {
        CurrentScene?.Clear();
    }
}
