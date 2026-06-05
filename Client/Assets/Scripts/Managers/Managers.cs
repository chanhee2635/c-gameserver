using UnityEngine;

public class Managers : MonoBehaviour
{
    private const int MaxPacketsPerFrame = 100;
    private const int ForegroundFrameRate = 45;
    private const int BackgroundFrameRate = 30;   

    private static Managers s_instance;
    private static bool s_isQuitting;

    public static Managers Instance
    {
        get
        {
            if (s_isQuitting) return null;
            if (s_instance == null) Bootstrap();
            return s_instance;
        }
    }

    public static WebManager Web => s_instance._web;
    public static NetworkManager  Network  => s_instance._network;
    public static UIManager       UI       => s_instance._ui;
    public static ResourceManager Resource => s_instance._resource;
    public static SoundManager    Sound    => s_instance._sound;
    public static SceneManagerEx  Scene    => s_instance._scene;
    public static DataManager     Data     => s_instance._data;
    public static PoolManager     Pool     => s_instance._pool;
    public static ObjectManager   Object   => s_instance._object;

    private WebManager _web  = new();
    private NetworkManager  _network  = new();
    private UIManager       _ui       = new();
    private ResourceManager _resource = new();
    private SoundManager    _sound    = new();
    private SceneManagerEx  _scene    = new();
    private DataManager     _data     = new();
    private PoolManager     _pool     = new();
    private ObjectManager   _object   = new();

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
    private static void Bootstrap()
    {
        if (s_instance != null) return;

        GameObject go = new GameObject("@Managers");
        DontDestroyOnLoad(go);
        s_instance = go.AddComponent<Managers>();
        s_instance.Init();
    }

    private void Init()
    {
        QualitySettings.vSyncCount = 0;
        Application.targetFrameRate = ForegroundFrameRate;
        Screen.SetResolution(960, 540, false);

        _data.Init();
        _sound.Init();
        _pool.Init();
    }

    private void Update()
    {
        _network.Update();
        PacketQueue.Instance.Flush(MaxPacketsPerFrame);
        _object.Update();
    }

    private void OnApplicationFocus(bool hasFocus)
    {
        Application.targetFrameRate = hasFocus ? ForegroundFrameRate : BackgroundFrameRate;
    }

    private void OnApplicationQuit()
    {
        s_isQuitting = true;
        Clear();
    }

    public void ClearScene()
    {
        _ui?.Clear();     
        _scene?.Clear();
        _object?.Clear();
        _sound?.Clear();
        _pool?.Clear();
    }

    public void Clear()
    {
        _network?.Clear();
        _sound?.Clear();
        _pool?.Clear();
    }

    private void OnDestroy()
    {
        if (s_instance == this)
            s_instance = null;
    }
}
