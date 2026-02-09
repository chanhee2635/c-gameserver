using Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectServerPopup_Item : UI_Base
{
    public UI_SelectServerPopup parent { get; set; }
    public ServerInfo Info { get; set; }

    enum Texts
    {
        ServerText
    }

    protected override void Init()
    {
        Bind<TextMeshProUGUI>(typeof(Texts));
        GetText((int)Texts.ServerText).gameObject.BindEvent(OnClickButton);
    }

    public void RefreshUI()
    {
        if (Info == null)
            return;

        GetText((int)Texts.ServerText).text = $"{Info.ServerName}({Info.SessionCount}/{Info.MaxCount})";
    }

    void OnClickButton(PointerEventData evt)
    {
        if (parent == null) return;

        parent.SelectServer(Info.ServerId);
    }

    public void SetColor(Color color)
    {
        GetText((int)Texts.ServerText).color = color;
    }
}
