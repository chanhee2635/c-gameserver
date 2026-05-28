using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_LoginScene : UI_Scene
{
    public UI_CreateUserPopup CreateUserPopup { get; private set; }

    enum Texts       { StatusText }
    enum InputFields { IDInput, PWInput }
    enum Buttons     { LoginBtn, CreateBtn, ExitBtn }

    protected override void Init()
    {
        base.Init();
        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.LoginBtn).gameObject.BindEvent(OnClickLoginButton);
        GetButton((int)Buttons.CreateBtn).gameObject.BindEvent(OnClickCreateButton);
    }

    private void OnClickCreateButton(PointerEventData evt)
    {
        if (CreateUserPopup == null)
            CreateUserPopup = Managers.UI.ShowPopupUI<UI_CreateUserPopup>();
    }

    private void OnClickLoginButton(PointerEventData evt)
    {
        GetText((int)Texts.StatusText).text = "로그인 시도 중...";

        string id = GetInputField((int)InputFields.IDInput).text;
        string pw = GetInputField((int)InputFields.PWInput).text;

        LoginAccountReq packet = new LoginAccountReq() { AccountName = id, Password = pw };
        Managers.Web.SendPostRequest<LoginAccountRes>("login", packet, (res) =>
        {
            if (res == null)
            {
                GetText((int)Texts.StatusText).text = "서버 연결 실패";
                return;
            }

            GetText((int)Texts.StatusText).text = res.Success ? "로그인 성공!" : "로그인 실패!";
            GetInputField((int)InputFields.IDInput).text = "";
            GetInputField((int)InputFields.PWInput).text = "";

            if (res.Success)
            {
                Managers.Network.AccountId = res.AccountId;
                Managers.Network.QueueToken = res.QueueToken;   
                Managers.Network.GateIp = res.GateIp;
                Managers.Network.GatePort = res.GatePort;

                UI_SelectServerPopup popup = Managers.UI.ShowPopupUI<UI_SelectServerPopup>();
                popup.SetServers(res.ServerList);
            }
        });
    }
}
