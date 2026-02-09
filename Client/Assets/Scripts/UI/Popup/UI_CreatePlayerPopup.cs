using Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_CreatePlayerPopup : UI_Popup
{
    int chooseClass = 1;

    enum Buttons
    {
        WarriorBtn,
        ThiefBtn,
        CreateBtn,
        ExitBtn
    }

    enum Texts
    {
        WarriorChk,
        ThiefChk,
        NamePlaceholder
    }

    enum InputFields
    {
        NameInput,
    }

    protected override void Init()
    {
        base.Init();

        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.WarriorBtn).gameObject.BindEvent(OnClickWarriorButton);
        GetButton((int)Buttons.ThiefBtn).gameObject.BindEvent(OnClickThiefButton);
        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
        GetButton((int)Buttons.ExitBtn).gameObject.BindEvent(OnClickExitButton);
    }

    public void OnClickWarriorButton(PointerEventData evt)
    {
        GetText((int)Texts.WarriorChk).text = "●";
        GetText((int)Texts.ThiefChk).text = "";
        chooseClass = 1;
    }

    public void OnClickThiefButton(PointerEventData evt)
    {
        GetText((int)Texts.ThiefChk).text = "●";
        GetText((int)Texts.WarriorChk).text = "";
        chooseClass = 2;
    }

    public void OnClickCreateButton(PointerEventData evt)
    {
        // class 값에 이상한 값을 보낼 때
        if (chooseClass < 1 || chooseClass > 2) return;

        // TODO : 정규표현식
        string name = GetInputField((int)InputFields.NameInput).text;
        if (string.IsNullOrEmpty(name))
        {
            SetWarningMessage("닉네임을 입력해주세요!");
            return;
        }

        C_CreatePlayer packet = new C_CreatePlayer();
        packet.TemplateId = chooseClass;
        packet.Name = GetInputField((int)InputFields.NameInput).text;
        Managers.Network.Send(packet);
    }

    public void OnClickExitButton(PointerEventData evt)
    {
        ClosePopupUI();
    }

    public void SetWarningMessage(string text)
    {
        GetInputField((int)InputFields.NameInput).text = "";
        GetText((int)Texts.NamePlaceholder).text = text;
    }
}
