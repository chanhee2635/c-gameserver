using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_CreateUserPopup : UI_Popup
{
    enum Texts       { StatusText }
    enum InputFields { IDInput, PWInput }
    enum Buttons     { JoinBtn, ExitBtn }

    protected override void Init()
    {
        base.Init();
        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<TMP_InputField>(typeof(InputFields));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.JoinBtn).gameObject.BindEvent(OnClickJoinButton);
        GetButton((int)Buttons.ExitBtn).gameObject.BindEvent(OnClickExitButton);
    }

    private void OnClickJoinButton(PointerEventData evt)
    {
        string id = GetInputField((int)InputFields.IDInput).text;
        string pw = GetInputField((int)InputFields.PWInput).text;

        CreateAccountReq packet = new CreateAccountReq() { AccountName = id, Password = pw };

        Managers.Web.SendPostRequest<CreateAccountRes>("create", packet, (res) =>
        {
            if (res == null)
            {
                GetText((int)Texts.StatusText).text = "서버 연결 실패";
                return;
            }

            GetText((int)Texts.StatusText).text = res.Success ? "회원가입 성공!" : "회원가입 실패!";
            GetInputField((int)InputFields.IDInput).text = "";
            GetInputField((int)InputFields.PWInput).text = "";
        });
    }

    private void OnClickExitButton(PointerEventData evt) => ClosePopupUI();
}
