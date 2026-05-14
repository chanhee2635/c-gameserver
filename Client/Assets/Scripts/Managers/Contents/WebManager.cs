using System;
using System.Collections;
using System.Text;
using UnityEngine;
using UnityEngine.Networking;

public class WebManager
{
    private const string BASE_URL = "http://127.0.0.1:5245/api/auth/"; // 로그인 서버 주소

    public void SendPostRequest<T>(string url, object obj, Action<T> callback)
    {
        string json = JsonUtility.ToJson(obj);
        Managers.Instance.StartCoroutine(CoSendWebRequest(url, json, callback));
    }

    private IEnumerator CoSendWebRequest<T>(string url, string json, Action<T> callback)
    {
        using var req = new UnityWebRequest(BASE_URL + url, "POST");
        byte[] body = Encoding.UTF8.GetBytes(json);
        req.uploadHandler   = new UploadHandlerRaw(body);
        req.downloadHandler = new DownloadHandlerBuffer();
        req.SetRequestHeader("Content-Type", "application/json");

        yield return req.SendWebRequest();

        if (req.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Web Error ({url}): {req.error}");
            callback?.Invoke(default);
        }
        else
        {
            T res = JsonUtility.FromJson<T>(req.downloadHandler.text);
            callback?.Invoke(res);
        }
    }
}
