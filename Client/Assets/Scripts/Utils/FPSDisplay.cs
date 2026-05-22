using UnityEngine;

public class FPSDisplay : MonoBehaviour
{
    private float    _deltaTime;
    private int      _lastDispatched;
    private float    _dispatchCheckTime;
    private float    _packetsPerSec;
    private GUIStyle _style;

    void Update()
    {
        _deltaTime += (Time.unscaledDeltaTime - _deltaTime) * 0.1f;
    }

    void OnGUI()
    {
        if (Time.time - _dispatchCheckTime >= 1f)
        {
            int cur = PacketQueue.Instance.TotalDispatched;
            _packetsPerSec = (cur - _lastDispatched) / (Time.time - _dispatchCheckTime);
            _lastDispatched = cur;
            _dispatchCheckTime = Time.time;
        }

        float fps = 1f / _deltaTime;

        if (_style == null)
        {
            _style = new GUIStyle(GUI.skin.box);
            _style.fontSize = 18;
            _style.alignment = TextAnchor.UpperLeft;
            _style.normal.background = Texture2D.blackTexture;
        }

        _style.normal.textColor = fps < 30f ? Color.red : Color.green;

        float w = 180f, h = 60f;
        GUI.Label(
            new Rect(Screen.width - w - 10, Screen.height - h - 10, w, h),
            $"FPS       : {fps:0.}\nPktPerSec : {_packetsPerSec:F0}",
            _style);
    }
}
