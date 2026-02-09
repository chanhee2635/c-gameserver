using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class UI_Summary : MonoBehaviour
{
    public CreatureController owner { get; set; }

    [SerializeField]
    Slider HpSlider;

    [SerializeField]
    TextMeshProUGUI InfoText;

    //bool ready = false;

    /*void Awake()
    {
        if (InfoText == null)
            InfoText = GetComponentInChildren<TextMeshProUGUI>(true);
        if (HpSlider == null)
            HpSlider = GetComponentInChildren<Slider>(true);

        ready = InfoText != null && HpSlider != null;
    }*/

    void Update()
    {
        if (owner == null) return;

        transform.rotation = Camera.main.transform.rotation;
        InfoText.text = $"Lv.{owner.Level} {owner.Name}";
        HpSlider.value = Mathf.Lerp(HpSlider.value, owner.GetHpRatio(), Time.deltaTime * 20f);
    }
}
