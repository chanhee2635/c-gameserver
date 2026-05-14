using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class UI_Minimap : MonoBehaviour
{
    [Header("UI References")]
    [SerializeField] private RectTransform _iconContainer;
    [SerializeField] private RectTransform _myIcon;

    [Header("Settings")]
    [SerializeField] private float  _zoomRange      = 30f;
    [SerializeField] private float  _uiSize         = 300f;
    [SerializeField] private string _iconPrefabName = "UI_OtherIcon";

    private const float UPDATE_INTERVAL = 0.05f;

    private float _nextUpdateTime;
    private float _sqrZoomRange;
    private float _uiRatio;

    private Dictionary<ulong, RectTransform> _iconDict = new Dictionary<ulong, RectTransform>();

    private void Awake()
    {
        _sqrZoomRange = _zoomRange * _zoomRange;
        _uiRatio      = _uiSize / (_zoomRange * 2f);
    }

    private void Update()
    {
        if (Time.time < _nextUpdateTime) return;
        _nextUpdateTime = Time.time + UPDATE_INTERVAL;

        UpdateMinimap();
    }

    private void UpdateMinimap()
    {
        MyPlayerController myPlayer = Managers.Object.MyPlayer;
        if (myPlayer == null) return;

        float camYaw = Camera.main.transform.eulerAngles.y;
        _iconContainer.localRotation = Quaternion.Euler(0, 0, camYaw);

        float myYaw = myPlayer.transform.eulerAngles.y;
        _myIcon.localRotation = Quaternion.Euler(0, 0, -myYaw);

        UpdateOtherIcons(myPlayer, camYaw);
    }

    private void UpdateOtherIcons(CreatureController myPlayer, float camYaw)
    {
        Vector3                  myPos     = myPlayer.transform.position;
        List<CreatureController> creatures = Managers.Object.GetCachedCreatures();

        for (int i = 0; i < creatures.Count; i++)
        {
            CreatureController cc = creatures[i];
            if (cc == null || cc == myPlayer) continue;

            ulong objId = cc.ObjectId;

            Vector3 offset = cc.transform.position - myPos;

            if (Mathf.Abs(offset.x) > _zoomRange || Mathf.Abs(offset.z) > _zoomRange)
            {
                DisableIcon(objId);
                continue;
            }

            if (offset.sqrMagnitude <= _sqrZoomRange)
            {
                RectTransform icon = GetOrMakeIcon(cc, objId);
                if (icon == null) continue;

                if (!icon.gameObject.activeSelf) icon.gameObject.SetActive(true);

                icon.anchoredPosition = new Vector2(offset.x * _uiRatio, offset.z * _uiRatio);
                icon.localRotation    = Quaternion.Euler(0, 0, -cc.transform.eulerAngles.y);
            }
        }
    }

    public void ShowIcon(ulong id, bool show)
    {
        if (_iconDict.TryGetValue(id, out RectTransform icon))
        {
            if (icon.gameObject.activeSelf != show)
                icon.gameObject.SetActive(show);
        }
    }

    private void DisableIcon(ulong id)
    {
        if (_iconDict.TryGetValue(id, out RectTransform icon))
        {
            if (icon.gameObject.activeSelf)
                icon.gameObject.SetActive(false);
        }
    }

    public void RemoveIcon(ulong id)
    {
        if (_iconDict.TryGetValue(id, out RectTransform icon))
        {
            Managers.Resource.Destroy(icon.gameObject);
            _iconDict.Remove(id);
        }
    }

    private RectTransform GetOrMakeIcon(CreatureController cc, ulong id)
    {
        if (_iconDict.TryGetValue(id, out RectTransform icon))
            return icon;

        GameObject go = Managers.Resource.Instantiate(_iconPrefabName, _iconContainer);
        if (go == null) return null;

        RectTransform rect = go.GetComponent<RectTransform>();
        rect.localScale = Vector3.one;

        Image img = go.GetComponent<Image>();
        if (img != null)
        {
            img.color = (cc.ObjectType == Protocol.GameObjectType.Player) ? Color.white : Color.red;
        }

        _iconDict.Add(id, rect);

        if (_myIcon != null)
            _myIcon.SetAsLastSibling();

        return rect;
    }
}
