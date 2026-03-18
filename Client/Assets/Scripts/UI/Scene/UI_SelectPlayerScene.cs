using Protocol;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectPlayerScene : UI_Scene
{
    UI_CreatePlayerPopup _popup = null;
    Dictionary<string, UI_SelectPlayerScene_Item> _items = new Dictionary<string, UI_SelectPlayerScene_Item>();
    string _selectPlayerName = "";

    enum Images { PlayerInfo_1, PlayerInfo_2, PlayerInfo_3, Count }
    enum Buttons{ CreateBtn, EnterBtn, ExitBtn }

    protected override void Init()
    {
        base.Init();
        Bind<Image>(typeof(Images));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
        GetButton((int)Buttons.EnterBtn).gameObject.BindEvent(OnClickEnterButton);
    }

    public void SetPlayerSummaries(List<PlayerSummary> summaries)
    {
        for (int i = 0; i< (int)Images.Count; ++i)
        {
            if (i < summaries.Count)
                AddPlayerItem(i, summaries[i]);
        }
    }

    void AddPlayerItem(int index, PlayerSummary summary)
    {
        GameObject go = Managers.Resource.Instantiate("UI/SubItem/UI_SelectPlayerScene_Item", GetImage(index).transform);
        UI_SelectPlayerScene_Item item = go.GetOrAddComponent<UI_SelectPlayerScene_Item>();
        item.Summary = summary;
        item.Parent = this;
        item.SetPlayerInfo();

        _items[summary.Name] = item;
    }

    void OnClickCreateButton(PointerEventData evt)
    {
        if (_popup == null)
            _popup = Managers.UI.ShowPopupUI<UI_CreatePlayerPopup>();
    }

    void OnClickEnterButton(PointerEventData evt)
    {
        if (_selectPlayerName == "") return;

        C_EnterGame packet = new C_EnterGame();
        packet.Name = _selectPlayerName;
        Managers.Network.Send(packet);
    }

    public void SelectPlayer(string playerName)
    {
        _selectPlayerName = playerName;
        foreach (var item in _items)
            item.Value.SetColor(item.Key == playerName ? Color.yellow : Color.white);
    }

    public void SetCreateResult(PlayerSummary summary)
    {
        _popup.ClosePopupUI();
        AddPlayerItem(_items.Count, summary);
    }

    public void SetWarningMessage(string message)
    {
        _popup.SetWarningMessage(message);
    }
}
