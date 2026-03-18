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
    int _chooseTemplateId = -1;

    enum Buttons { WarriorBtn, ThiefBtn, CreateBtn, ExitBtn }
    enum Texts { WarriorChk, ThiefChk, NamePlaceholder }
    enum InputFields { NameInput }

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

    void SelectClass(int templateId)
    {
        _chooseTemplateId = templateId;
        GetText((int)Texts.WarriorChk).text = "";
        GetText((int)Texts.ThiefChk).text = "";

        PrefabData data = Managers.Data.PrefabDataDict[templateId];
        if (templateId == 1) GetText((int)Texts.WarriorChk).text = "●";
        else if (templateId == 2) GetText((int)Texts.ThiefChk).text = "●";
    }

    public void OnClickWarriorButton(PointerEventData evt) { SelectClass(1); }
    public void OnClickThiefButton(PointerEventData evt) { SelectClass(2); }
    public void OnClickCreateButton(PointerEventData evt)
    {
        if (_chooseTemplateId == -1) return;

        string name = GetInputField((int)InputFields.NameInput).text;
        if (string.IsNullOrEmpty(name))
        {
            SetWarningMessage("닉네임을 입력해주세요!");
            return;
        }

        C_CreatePlayer packet = new C_CreatePlayer();
        packet.TemplateId = _chooseTemplateId;
        packet.Name = name;
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
