using Protocol;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_GameScene : UI_Scene
{
    public List<UI_Chat_Item> chats { get; } = new List<UI_Chat_Item>();

    enum Texts
    {
        LevelText
    }

    enum Sliders
    {
        HPSlider,
        MPSlider,
        EXPSlider
    }

    enum ChatItem
    {
        ChatItem
    }

    enum Dropdowns
    {
        ChatType
    }

    enum InputFields
    {
        ChatInput
    }

    enum Buttons
    {
        AttackBtn,
        SendBtn,
        CurrentPosBtn,
        VillagePosBtn,
        YesBtn,
        NoBtn
    }

    enum Images
    {
        AttackCool,
        Revive,
        Exit
    }

    protected override void Init()
    {
        base.Init();

        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<Slider>(typeof(Sliders));
        Bind<Button>(typeof(Buttons));
        Bind<Image>(typeof(Images));
        Bind<TMP_Dropdown>(typeof(Dropdowns));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<VerticalLayoutGroup>(typeof(ChatItem));

        GetButton((int)Buttons.SendBtn).gameObject.BindEvent(OnClickSendButton);
        GetButton((int)Buttons.CurrentPosBtn).gameObject.BindEvent(OnClickCurrentPosButton);
        GetButton((int)Buttons.VillagePosBtn).gameObject.BindEvent(OnClickVillagePosButton);
        GetButton((int)Buttons.YesBtn).gameObject.BindEvent(OnClickExitYesButton);
        GetButton((int)Buttons.NoBtn).gameObject.BindEvent(OnClickExitNoButton);

        foreach (Transform child in Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform.Cast<Transform>().ToList())
            Managers.Resource.Destroy(child.gameObject);

        GetImage((int)Images.Revive).gameObject.SetActive(false);
        GetImage((int)Images.Exit).gameObject.SetActive(false);

        PlayerSetAttackCool();
    }

    void Update()
    {
        GetText((int)Texts.LevelText).text = Managers.Object.MyPlayer.Level.ToString();
        Get<Slider>((int)Sliders.HPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.HPSlider).value, Managers.Object.MyPlayer.GetHpRatio(), Time.deltaTime * 20f);
        Get<Slider>((int)Sliders.MPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.MPSlider).value, Managers.Object.MyPlayer.GetMpRatio(), Time.deltaTime * 20f);
        Get<Slider>((int)Sliders.EXPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.EXPSlider).value, Managers.Object.MyPlayer.GetExpRatio(), Time.deltaTime * 20f);

        if (Input.GetKeyDown(KeyCode.Escape))
        {
            ShowExitPopup();
        }
    }

    public void OnClickSendButton(PointerEventData evt)
    {
        SendChatting();
    }

    public void OnClickCurrentPosButton(PointerEventData evt)
    {
        SendRevivePacket(true);
    }

    public void OnClickVillagePosButton(PointerEventData evt)
    {
        SendRevivePacket(false);
    }

    public void OnClickExitYesButton(PointerEventData evt)
    {
        C_Quit packet = new C_Quit();
        Managers.Network.Send(packet);
        Managers.Network.DisconnectToChatServer();
        Managers.Scene.LoadScene(Define.Scene.Login);
    }

    public void OnClickExitNoButton(PointerEventData evt)
    {
        GetImage((int)Images.Exit).gameObject.SetActive(false);
    }

    private void SendRevivePacket(bool isCurrentPos)
    {
        if (Managers.Object.MyPlayer.State != CreatureState.Dead) return;

        C_Revive packet = new C_Revive();
        packet.IsCurrentPos = isCurrentPos;
        Managers.Network.Send(packet);

        GetImage((int)Images.Revive).gameObject.SetActive(false);
    }

    public void SendChatting()
    {
        string msg = Get<TMP_InputField>((int)InputFields.ChatInput).text.Trim();
        if (string.IsNullOrEmpty(msg)) return;

        C_Chat packet = new C_Chat();
        packet.ToServer = Get<TMP_Dropdown>((int)Dropdowns.ChatType).value == 1;
        packet.Msg = msg;
        Managers.Network.SendToChat(packet);

        Get<TMP_InputField>((int)InputFields.ChatInput).text = null;
    }

    public void RecvChatting(S_Chat packet)
    {
        if (chats.Count > 20)
        {
            Managers.Resource.Destroy(chats[0].gameObject);
            chats.RemoveAt(0);
        }

        string type = "일반";
        Color color = Color.black;
        if (packet.ToServer)
        {
            type = "서버";
            color = Color.green;
        }

        string msg = $"[{type}] {packet.Name} : {packet.Msg}";

        UI_Chat_Item item = Managers.UI.MakeSubItem<UI_Chat_Item>(Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform);
        item.transform.localScale = Vector3.one;
        item.SetText(msg, color);
        chats.Add(item);
    }

    public void PlayerSetAttackCool()
    {
        Managers.Object.MyPlayer.SetAttackCool(GetImage((int)Images.AttackCool));
    }

    public void ShowRevivePopup()
    {
        if (Managers.Object.MyPlayer.State != CreatureState.Dead) return;

        GetImage((int)Images.Revive).gameObject.SetActive(true);
    }

    public void ShowExitPopup()
    {
        GameObject go = GetImage((int)Images.Exit).gameObject;
        go.SetActive(!go.activeSelf);
    }
}
