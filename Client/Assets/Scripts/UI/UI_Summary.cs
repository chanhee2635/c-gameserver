using System;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class UI_Summary : MonoBehaviour
{
    public CreatureController Owner { get; set; }

    [SerializeField] private Slider           HpSlider;
    [SerializeField] private TextMeshProUGUI  InfoText;

    private Camera _mainCam;
    private Transform _followTarget;
    private float _targetRatio;

    public void SetFollowTarget(Transform target) => _followTarget = target;

    private void Start()
    {
        _mainCam = Camera.main;
    }

    private void OnEnable()
    {
        RefreshInfo();
        if (Owner != null && HpSlider != null)
            HpSlider.value = Owner.GetHpRatio();
    }

    public void RefreshInfo()
    {
        if (Owner == null) return;

        if (InfoText != null)
            InfoText.text = $"Lv.{Owner.Level} {Owner.Name}";

        _targetRatio = Owner.GetHpRatio();
    }

    private void Update()
    {
        if (Owner == null) return;

        if (_followTarget != null)
            transform.position = _followTarget.position;

        if (_mainCam != null)
            transform.rotation = _mainCam.transform.rotation;

        UpdateHpBar();
    }

    private void UpdateHpBar()
    {
        if (HpSlider == null) return;

        float currentActualRatio = Owner.GetHpRatio();

        if (Mathf.Abs(HpSlider.value - currentActualRatio) < 0.001f)
        {
            HpSlider.value = currentActualRatio;
            return;
        }

        HpSlider.value = Mathf.Lerp(HpSlider.value, currentActualRatio, Time.deltaTime * 10f);
    }
}
