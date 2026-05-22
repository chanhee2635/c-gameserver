using System;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_GameScene : UI_Scene
{
    public List<UI_Chat_Item> Chats { get; } = new List<UI_Chat_Item>();

    enum Texts       { LevelText }
    enum Sliders     { HPSlider, MPSlider, EXPSlider }
    enum ChatItem    { ChatItem }
    enum Dropdowns   { ChatType }
    enum InputFields { ChatInput }
    enum Buttons     { AttackBtn, SendBtn, CurrentReviveBtn, NearbyReviveBtn, ExitYesBtn, ExitNoBtn }
    enum Images      { AttackCool, RevivePopup, ExitPopup, UI_Minimap }

    public UI_Minimap Minimap { get; private set; }

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
        GetButton((int)Buttons.CurrentReviveBtn).gameObject.BindEvent(_ => SendRevivePacket(isCurrentPos: true));
        GetButton((int)Buttons.NearbyReviveBtn).gameObject.BindEvent(_ => SendRevivePacket(isCurrentPos: false));
        GetButton((int)Buttons.ExitYesBtn).gameObject.BindEvent(OnClickExitYesButton);
        GetButton((int)Buttons.ExitNoBtn).gameObject.BindEvent(OnClickExitNoButton);
        GetButton((int)Buttons.AttackBtn).gameObject.BindEvent(OnClickAttackButton);

        GameObject minimapGo = GetImage((int)Images.UI_Minimap).gameObject;
        if (minimapGo != null)
            Minimap = minimapGo.GetComponent<UI_Minimap>();

        foreach (Transform child in Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform.Cast<Transform>().ToList())
            Managers.Resource.Destroy(child.gameObject);

        GetImage((int)Images.RevivePopup).gameObject.SetActive(false);
        GetImage((int)Images.ExitPopup).gameObject.SetActive(false);
    }

    public void SetMyPlayerInfo()
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;

        GetText((int)Texts.LevelText).text    = player.Level.ToString();
        Get<Slider>((int)Sliders.HPSlider).value  = player.GetHpRatio();
        Get<Slider>((int)Sliders.MPSlider).value  = player.GetMpRatio();
        Get<Slider>((int)Sliders.EXPSlider).value = player.GetExpRatio();

        player.SetAttackCool(GetImage((int)Images.AttackCool));
    }

    private void Update()
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;

        GetText((int)Texts.LevelText).text = player.Level.ToString();
        Get<Slider>((int)Sliders.HPSlider).value  = Mathf.Lerp(Get<Slider>((int)Sliders.HPSlider).value,  player.GetHpRatio(),  Time.deltaTime * 20f);
        Get<Slider>((int)Sliders.MPSlider).value  = Mathf.Lerp(Get<Slider>((int)Sliders.MPSlider).value,  player.GetMpRatio(),  Time.deltaTime * 20f);
        Get<Slider>((int)Sliders.EXPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.EXPSlider).value, player.GetExpRatio(), Time.deltaTime * 20f);

        if (Input.GetKeyDown(KeyCode.Escape))
            ShowExitPopup();

        if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.KeypadEnter))
            HandleChatInput();
    }

    private void OnClickSendButton(PointerEventData evt)    => SendChatting();
    private void OnClickExitNoButton(PointerEventData evt)  => GetImage((int)Images.ExitPopup).gameObject.SetActive(false);
    private void OnClickAttackButton(PointerEventData evt)  => Managers.Object.MyPlayer?.TryAttack();

    private void OnClickExitYesButton(PointerEventData evt)
    {
        GetImage((int)Images.ExitPopup).gameObject.SetActive(false);
        Managers.Network.Send(new Protocol.CLeaveGame());
        Managers.Network.Clear();
    }

    private void SendRevivePacket(bool isCurrentPos)
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null || player.State != Protocol.CreatureState.Dead) return;

        Managers.Network.Send(new Protocol.CRevive { IsCurrentPos = isCurrentPos });
        GetImage((int)Images.RevivePopup).gameObject.SetActive(false);
    }

    private void HandleChatInput()
    {
        TMP_InputField chatInput = Get<TMP_InputField>((int)InputFields.ChatInput);

        if (EventSystem.current.currentSelectedGameObject != chatInput.gameObject)
        {
            chatInput.ActivateInputField();
        }
        else
        {
            if (!string.IsNullOrEmpty(chatInput.text))
                SendChatting();
            chatInput.text = "";
            EventSystem.current.SetSelectedGameObject(null);
        }
    }

    public void SendChatting()
    {
        string msg = Get<TMP_InputField>((int)InputFields.ChatInput).text.Trim();
        if (string.IsNullOrEmpty(msg)) return;

        int typeIndex = Get<TMP_Dropdown>((int)Dropdowns.ChatType).value;
        Protocol.ChatType chatType = typeIndex == 1
            ? Protocol.ChatType.ChatWorld
            : Protocol.ChatType.ChatNear;

        Managers.Network.Send(new Protocol.CChat { Chat = msg, ChatType = chatType });
        Get<TMP_InputField>((int)InputFields.ChatInput).text = null;
    }

    public void RecvChatting(Protocol.SChat packet)
    {
        if (Chats.Count > 20)
        {
            Managers.Resource.Destroy(Chats[0].gameObject);
            Chats.RemoveAt(0);
        }

        bool isWorld = packet.ChatType == Protocol.ChatType.ChatWorld;
        string prefix = isWorld ? "[ÀüÃ¼] " : "";
        string msg = $"{prefix}{packet.Name} : {packet.Chat}";
        Color color = isWorld ? Color.blue : Color.black;

        UI_Chat_Item item = Managers.UI.MakeSubItem<UI_Chat_Item>(Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform);
        item.transform.localScale = Vector3.one;
        item.SetText(msg, color);
        Chats.Add(item);
    }

    public void ShowRevivePopup()
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null || player.State != Protocol.CreatureState.Dead) return;

        GetImage((int)Images.RevivePopup).gameObject.SetActive(true);
    }

    public void ShowExitPopup()
    {
        GameObject popup = GetImage((int)Images.ExitPopup).gameObject;
        popup.SetActive(!popup.activeSelf);
    }

    public void HideReviveUI() => GetImage((int)Images.RevivePopup).gameObject.SetActive(false);
}
