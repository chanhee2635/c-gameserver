using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_GameScene : UI_Scene
{
    public List<UI_Chat_Item> chats { get; } = new List<UI_Chat_Item>();

    enum Texts { LevelText }
    enum Sliders { HPSlider, MPSlider, EXPSlider }
    enum ChatItem { ChatItem }
    enum Dropdowns { ChatType }
    enum InputFields { ChatInput }
    enum Buttons { AttackBtn, SendBtn, CurrentReviveBtn, NearbyReviveBtn, ExitYesBtn, ExitNoBtn }
    enum Images { AttackCool, RevivePopup, ExitPopup }

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

        foreach (Transform child in Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform.Cast<Transform>().ToList())
            Managers.Resource.Destroy(child.gameObject);

        GetImage((int)Images.RevivePopup).gameObject.SetActive(false);
        GetImage((int)Images.ExitPopup).gameObject.SetActive(false);
    }

    public void SetMyPlayerInfo()
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;

        GetText((int)Texts.LevelText).text = player.Level.ToString();
        Get<Slider>((int)Sliders.HPSlider).value = player.GetHpRatio();
        Get<Slider>((int)Sliders.MPSlider).value = player.GetMpRatio();
        Get<Slider>((int)Sliders.EXPSlider).value = player.GetExpRatio();

        player.SetAttackCool(GetImage((int)Images.AttackCool));
    }

    void Update()
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;

        GetText((int)Texts.LevelText).text = player.Level.ToString();
        Get<Slider>((int)Sliders.HPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.HPSlider).value, player.GetHpRatio(), Time.deltaTime * 20f); ;
        Get<Slider>((int)Sliders.MPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.MPSlider).value, player.GetMpRatio(), Time.deltaTime * 20f); ;
        Get<Slider>((int)Sliders.EXPSlider).value = Mathf.Lerp(Get<Slider>((int)Sliders.EXPSlider).value, player.GetExpRatio(), Time.deltaTime * 20f); ;

        if (Input.GetKeyDown(KeyCode.Escape))
            ShowExitPopup();
    }

    public void OnClickSendButton(PointerEventData evt) { SendChatting(); }
    public void OnClickExitYesButton(PointerEventData evt)
    {
        GetImage((int)Images.ExitPopup).gameObject.SetActive(false);

        Managers.Network.Send(new Protocol.C_Quit());
        Managers.Network.DisconnectToChatServer();
        Managers.Scene.LoadScene(Define.Scene.Login);
    }

    public void OnClickExitNoButton(PointerEventData evt)
    {
        GetImage((int)Images.ExitPopup).gameObject.SetActive(false);
    }

    public void OnClickAttackButton(PointerEventData evt)
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null) return;
        player.TryAttack();
    }

    private void SendRevivePacket(bool isCurrentPos)
    {
        MyPlayerController player = Managers.Object.MyPlayer;
        if (player == null || player.State != Protocol.CreatureState.Dead) return;

        Managers.Network.Send(new Protocol.C_Revive { IsCurrentPos = isCurrentPos });
        GetImage((int)Images.RevivePopup).gameObject.SetActive(false);
    }

    public void SendChatting()
    {
        string msg = Get<TMP_InputField>((int)InputFields.ChatInput).text.Trim();
        if (string.IsNullOrEmpty(msg)) return;

        Managers.Network.SendToChat(new Protocol.C_Chat
        {
            ToServer = Get<TMP_Dropdown>((int)Dropdowns.ChatType).value == 1,
            Msg = msg
        });

        Get<TMP_InputField>((int)InputFields.ChatInput).text = null;
    }

    public void RecvChatting(Protocol.S_Chat packet)
    {
        if (chats.Count > 20)
        {
            Managers.Resource.Destroy(chats[0].gameObject);
            chats.RemoveAt(0);
        }

        string type = packet.ToServer ? "서버" : "일반";
        Color color = packet.ToServer ? Color.green : Color.black;
        string msg = $"[{type}] {packet.Name} : {packet.Msg}";

        UI_Chat_Item item = Managers.UI.MakeSubItem<UI_Chat_Item>(Get<VerticalLayoutGroup>((int)ChatItem.ChatItem).transform);
        item.transform.localScale = Vector3.one;
        item.SetText(msg, color);
        chats.Add(item);
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
}
