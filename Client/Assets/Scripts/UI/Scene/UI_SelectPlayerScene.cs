using Protocol;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectPlayerScene : UI_Scene
{
    private UI_CreatePlayerPopup _popup;
    private Dictionary<string, UI_SelectPlayerScene_Item> _items = new();
    private string _selectPlayerName = "";

    enum Images  { PlayerInfo_1, PlayerInfo_2, PlayerInfo_3, Count }
    enum Buttons { CreateBtn, EnterBtn, ExitBtn }

    protected override void Init()
    {
        base.Init();
        Bind<Image>(typeof(Images));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
        GetButton((int)Buttons.EnterBtn).gameObject.BindEvent(OnClickEnterButton);
    }

    public void SetPlayerSummaries(IEnumerable<PlayerSummary> summaries)
    {
        int index = 0;
        foreach (var summary in summaries)
        {
            if (index >= (int)Images.Count) break;
            AddPlayerItem(index++, summary);
        }
    }

    private void AddPlayerItem(int index, PlayerSummary summary)
    {
        GameObject go = Managers.Resource.Instantiate("UI/SubItem/UI_SelectPlayerScene_Item", GetImage(index).transform);
        var item      = go.GetOrAddComponent<UI_SelectPlayerScene_Item>();
        item.Summary = summary;
        item.Parent  = this;
        item.SetPlayerInfo();
        _items[summary.Name] = item;
    }

    private void OnClickCreateButton(PointerEventData evt)
    {
        if (_popup == null)
            _popup = Managers.UI.ShowPopupUI<UI_CreatePlayerPopup>();
    }

    private void OnClickEnterButton(PointerEventData evt)
    {
        if (string.IsNullOrEmpty(_selectPlayerName)) return;

        Managers.Network.Send(new CEnterGame { Name = _selectPlayerName });
        Managers.Scene.LoadScene(Define.Scene.Game);
    }

    public void SelectPlayer(string playerName)
    {
        _selectPlayerName = playerName;
        foreach (var item in _items)
            item.Value.SetColor(item.Key == playerName ? UnityEngine.Color.yellow : UnityEngine.Color.white);
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
