using Protocol;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectPlayerScene_Item : UI_Base
{
    public UI_SelectPlayerScene Parent { get; set; }
    public PlayerSummary Summary { get; set; }

    enum RenderImages { PlayerImage }
    enum Texts{ LevelText, ClassText }

    protected override void Init()
    {
        Bind<RawImage>(typeof(RenderImages));
        Bind<TextMeshProUGUI>(typeof(Texts));

        gameObject.BindEvent(OnClickEvent);
    }

    void OnClickEvent(PointerEventData evt)
    {
        if (Parent == null || Summary == null) return;
        Parent.SelectPlayer(Summary.Name);
    }

    public void SetPlayerInfo()
    {
        if (Summary == null) return;

        PrefabData data = Managers.Data.PrefabDataDict[Summary.TemplateId];
        Get<RawImage>((int)RenderImages.PlayerImage).texture = Managers.Resource.Load<RenderTexture>($"RenderTexture/{data.prefabPath}");
        GetText((int)Texts.LevelText).text = $"Lv.{Summary.Level} {Summary.Name}";
        GetText((int)Texts.ClassText).text = data.name;
    }

    public void SetColor(Color color)
    {
        gameObject.GetOrAddComponent<Image>().color = color;
    }
}
