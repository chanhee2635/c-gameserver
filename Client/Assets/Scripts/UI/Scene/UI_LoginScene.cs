using Protocol;
using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_LoginScene : UI_Scene
{
    public UI_CreateUserPopup CreateUserPopup { get; private set; }

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
        LoginBtn,
        CreateBtn,
        ExitBtn
    }

    protected override void Init()
    {
        base.Init();

        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.LoginBtn).gameObject.BindEvent(OnClickLoginButton);
        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
    }

    public void OnClickLoginButton(PointerEventData evt)
    {
        string id = GetInputField((int)InputFields.IDInput).text;
        string pw = GetInputField((int)InputFields.PWInput).text;

        GetText((int)Texts.StatusText).text = "";

        // SSL/TLS 프로토콜 송신 필요 (해킹 위험)
        C_LoginAuth pkt = new C_LoginAuth() { ID = id, PW = pw };
        Managers.Network.SendToLogin(pkt);
    }

    public void OnClickCreateButton(PointerEventData evt)
    {
        // 아이디 패스워드를 입력하여 가입하는 창이 떠야한다.
        if (CreateUserPopup == null)
            CreateUserPopup = Managers.UI.ShowPopupUI<UI_CreateUserPopup>();
    }

    public void SetMessage(string text)
    {
        GetText((int)Texts.StatusText).text = text;
    }
}
