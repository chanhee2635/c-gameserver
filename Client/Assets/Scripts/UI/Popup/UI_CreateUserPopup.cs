using Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_CreateUserPopup : UI_Popup
{
    enum Texts
    {
        StatusText
    }

    enum InputFields
    {
        IDInput,
        PWInput
    }

    enum Buttons
    {
        JoinBtn,
        ExitBtn
    }

    protected override void Init()
    {
        base.Init();

        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.JoinBtn).gameObject.BindEvent(OnClickJoinButton);
        GetButton((int)Buttons.ExitBtn).gameObject.BindEvent(OnClickExitButton);
    }

    public void OnClickJoinButton(PointerEventData evt)
    {
        string id = GetInputField((int)InputFields.IDInput).text;
        string pw = GetInputField((int)InputFields.PWInput).text;

        // TODO: 비밀번호 재확인 필요 (우선 정확하다 판단)

        // SSL/TLS 프로토콜 송신 필요 (해킹 위험)
        C_Join pkt = new C_Join() { ID = id, PW = pw };
        Managers.Network.SendToLogin(pkt);
    }

    public void OnClickExitButton(PointerEventData evt)
    {
        ClosePopupUI();
    }

    public void SetJoinResult(bool success)
    {
        GetText((int)Texts.StatusText).text = success ? "계정 생성 성공!" : "계정 생성 실패!";
    }
}
