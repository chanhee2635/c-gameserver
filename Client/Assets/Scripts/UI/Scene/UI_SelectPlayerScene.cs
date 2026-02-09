using Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectPlayerScene : UI_Scene
{
    UI_CreatePlayerPopup popup = null;
    Dictionary<string, UI_SelectPlayerScene_Item> Items = new Dictionary<string, UI_SelectPlayerScene_Item>();
    string _selectPlayerName = "";

    enum Images
    {
        PlayerInfo_1,
        PlayerInfo_2,
        PlayerInfo_3,
        Count
    }

    enum Buttons
    {
        CreateBtn,
        EnterBtn,
        ExitBtn
    }

    protected override void Init()
    {
        base.Init();

        Bind<Image>(typeof(Images));
        Bind<Button>(typeof(Buttons));

        SetPlayerSummaries();

        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
        GetButton((int)Buttons.EnterBtn).gameObject.BindEvent(OnClickEnterButton);
    }

    void OnClickCreateButton(PointerEventData evt)
    {
        if (popup == null)
            popup = Managers.UI.ShowPopupUI<UI_CreatePlayerPopup>();
    }

    void OnClickEnterButton(PointerEventData evt)
    {
        if (_selectPlayerName == "") return;

        C_EnterGame packet = new C_EnterGame();
        packet.PlayerName = _selectPlayerName;
        Managers.Network.Send(packet);
    }

    void SetPlayerSummaries()
    {
        List<SummaryInfo> summaries = Managers.Object.Summaries;
        for (int i = 0; i < (int)Images.Count; i++)
        {
            if (summaries.Count > i)
                SetPlayerSummary(i, summaries[i]);
        }
    }

    void SetPlayerSummary(int idx, SummaryInfo summary)
    {
        GameObject go = Managers.Resource.Instantiate("UI/SubItem/UI_SelectPlayerScene_Item", GetImage(idx).transform);
        UI_SelectPlayerScene_Item item = go.GetOrAddComponent<UI_SelectPlayerScene_Item>();
        Items.Add(summary.Name, item);
        item.Info = summary;
        item.parent = this;
        item.SetPlayerInfo();
    }

    public void SelectPlayer(string playerName)
    {
        _selectPlayerName = playerName;

        foreach (var item in Items)
        {
            if (item.Key == playerName)
                item.Value.SetColor(Color.yellow);
            else
                item.Value.SetColor(Color.white);
        }
    }

    public void SetCreateResult(SummaryInfo summary)
    {
        popup.ClosePopupUI();
        SetPlayerSummary(Items.Count, summary);
    }

    public void SetWarningMessage(string message)
    {
        popup.SetWarningMessage(message);
    }
}
