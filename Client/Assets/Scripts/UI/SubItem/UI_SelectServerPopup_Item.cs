using Protocol;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectServerPopup_Item : UI_Base
{
    public UI_SelectServerPopup Parent { get; set; }
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

        GetText((int)Texts.ServerText).text = $"{Info.ServerName}({Info.CurrentCount}/{Info.MaxCount})";
    }

    private void OnClickButton(PointerEventData evt)
    {
        if (Parent == null) return;

        Parent.SelectServer(Info.Id);
    }

    public void SetColor(Color color)
    {
        GetText((int)Texts.ServerText).color = color;
    }
}
