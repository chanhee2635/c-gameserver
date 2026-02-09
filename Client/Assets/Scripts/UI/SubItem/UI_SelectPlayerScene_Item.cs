using Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectPlayerScene_Item : UI_Base
{
    public UI_SelectPlayerScene parent { get; set; }
    public SummaryInfo Info { get; set; }

    enum RenderImages
    {
        PlayerImage
    }

    enum Texts
    {
        LevelText,
        ClassText
    }

    protected override void Init()
    {
        Bind<RawImage>(typeof(RenderImages));
        Bind<TextMeshProUGUI>(typeof(Texts));

        gameObject.BindEvent(OnClickEvent);
    }

    void OnClickEvent(PointerEventData evt)
    {
        if (parent == null) return;
        if (Info == null) return;

        parent.SelectPlayer(Info.Name);
    }

    public void SetPlayerInfo()
    {
        if (Info != null)
        {
            PrefabData data = Managers.Data.PrefabDataDict[Info.TemplateId];
            Get<RawImage>((int)RenderImages.PlayerImage).texture = Managers.Resource.Load<RenderTexture>($"RenderTexture/{data.prefabPath}");
            GetText((int)Texts.LevelText).text = $"Lv.{Info.Level} {Info.Name}";
            GetText((int)Texts.ClassText).text = data.name;
        }
    }

    public void SetColor(Color color)
    {
        gameObject.GetOrAddComponent<Image>().color = color;
    }
}
