using Protocol;
using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_CreatePlayerPopup : UI_Popup
{
    private int _chooseTemplateId = -1;

    enum Buttons     { WarriorBtn, ThiefBtn, CreateBtn, ExitBtn }
    enum Texts       { WarriorChk, ThiefChk, NamePlaceholder }
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

    private void SelectClass(int templateId)
    {
        _chooseTemplateId = templateId;
        GetText((int)Texts.WarriorChk).text = "";
        GetText((int)Texts.ThiefChk).text   = "";

        var data = Managers.Data.PrefabDataDict[templateId];
        if (templateId == 1)      GetText((int)Texts.WarriorChk).text = "●";
        else if (templateId == 2) GetText((int)Texts.ThiefChk).text   = "●";
    }

    private void OnClickWarriorButton(PointerEventData evt) => SelectClass(1);
    private void OnClickThiefButton(PointerEventData evt)   => SelectClass(2);

    private void OnClickCreateButton(PointerEventData evt)
    {
        if (_chooseTemplateId == -1) return;

        string name = GetInputField((int)InputFields.NameInput).text;
        if (string.IsNullOrEmpty(name))
        {
            SetWarningMessage("캐릭터 이름을 입력하세요!");
            return;
        }

        Managers.Network.Send(new CCreatePlayer { TemplateId = _chooseTemplateId, Name = name });
    }

    private void OnClickExitButton(PointerEventData evt) => ClosePopupUI();

    public void SetWarningMessage(string text)
    {
        GetInputField((int)InputFields.NameInput).text = "";
        GetText((int)Texts.NamePlaceholder).text = text;
    }
}
