using Google.Protobuf.Collections;
using Protocol;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_SelectServerPopup : UI_Popup
{
    public Dictionary<int, UI_SelectServerPopup_Item> Items { get; } = new Dictionary<int, UI_SelectServerPopup_Item>();
    private int _selectServerId { get; set; }

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
    public void SetServerList(RepeatedField<ServerInfo> serverList)
    {
        Items.Clear();

        GameObject grid = GetComponentInChildren<VerticalLayoutGroup>().gameObject;
        foreach (Transform child in grid.transform)
            Destroy(child.gameObject);

        for (int i = 0; i < serverList.Count; i++)
        {
            GameObject go = Managers.Resource.Instantiate("UI/SubItem/UI_SelectServerPopup_Item", grid.transform);
            UI_SelectServerPopup_Item item = go.GetOrAddComponent<UI_SelectServerPopup_Item>();
            Items.Add(serverList[i].ServerId, item);
            item.parent = this;
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

    void OnClickExitButton(PointerEventData evt)
    {
        // TODO 다시 로그인? 토큰활용
        ClosePopupUI();
    }

    void OnClickEnterButton(PointerEventData evt)
    {
        if (_selectServerId == 0) return;

        ServerInfo info = Items[_selectServerId].Info;
        if (info == null) return;

        Managers.Network.DisconnectToLoginServer();
        Managers.Network.ConnectToGame(info);
    }

}
