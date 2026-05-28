using Protocol;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectServerPopup : UI_Popup
{
    public Dictionary<int, UI_SelectServerPopup_Item> Items { get; } = new Dictionary<int, UI_SelectServerPopup_Item>();
    private int _selectServerId;

    enum Buttons
    {
        ExitBtn,
        EnterBtn
    }

    protected override void Init()
    {
        base.Init();

        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.ExitBtn).gameObject.BindEvent(OnClickExitButton);
        GetButton((int)Buttons.EnterBtn).gameObject.BindEvent(OnClickEnterButton);
    }

    public void SetServers(List<ServerInfo> serverList)
    {
        Items.Clear();

        GameObject grid = GetComponentInChildren<VerticalLayoutGroup>().gameObject;
        foreach (Transform child in grid.transform)
            Destroy(child.gameObject);

        for (int i = 0; i < serverList.Count; i++)
        {
            GameObject go = Managers.Resource.Instantiate("UI/SubItem/UI_SelectServerPopup_Item", grid.transform);
            UI_SelectServerPopup_Item item = go.GetOrAddComponent<UI_SelectServerPopup_Item>();
            Items.Add(serverList[i].Id, item);
            item.Parent = this;
            item.Info = serverList[i];
        }

        RefreshUI();
    }

    public void RefreshUI()
    {
        if (Items.Count == 0) return;

        foreach (var item in Items)
        {
            item.Value.RefreshUI();
        }
    }

    public void SelectServer(int serverId)
    {
        _selectServerId = serverId;

        foreach (var item in Items)
        {
            if (item.Key == serverId)
                item.Value.SetColor(Color.red);
            else
                item.Value.SetColor(Color.black);
        }
    }

    private void OnClickExitButton(PointerEventData evt) => ClosePopupUI();

    private void OnClickEnterButton(PointerEventData evt)
    {
        if (_selectServerId == 0) return;
        ServerInfo info = Items[_selectServerId].Info;
        if (info == null) return;

        Managers.UI.ShowPopupUI<UI_QueuePopup>();   
        Managers.Network.ConnectToGateway(info.Id);
    }
}
