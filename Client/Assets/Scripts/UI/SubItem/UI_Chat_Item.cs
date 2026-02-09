using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class UI_Chat_Item : UI_Base
{
    TextMeshProUGUI _text;

    protected override void Init()
    {
        _text = GetComponent<TextMeshProUGUI>();
    }

    public void SetText(string text, Color color)
    {
        _text.text = text;
        _text.color = color;
    }
}
