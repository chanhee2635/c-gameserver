using TMPro;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_QueuePopup : UI_Popup
{
    enum Texts { StatusText }
    enum Buttons { CancelBtn }

    protected override void Init()
    {
        base.Init();
        Bind<TextMeshProUGUI>(typeof(Texts));
        Bind<Button>(typeof(Buttons));

        GetButton((int)Buttons.CancelBtn).gameObject.BindEvent(OnClickCancel);

        Managers.Network.OnQueueStatus += OnQueueStatus;
        Managers.Network.OnQueueAdmitted += OnAdmitted;
        Managers.Network.OnQueueRejected += OnRejected;

        GetText((int)Texts.StatusText).text = "대기열 등록 중...";
    }

    private void OnQueueStatus(int position, int total)
        => GetText((int)Texts.StatusText).text = $"대기 중 - {position}번째 / 전체 {total}명";

    private void OnAdmitted() { Unsubscribe(); ClosePopupUI(); }   
    private void OnRejected(string reason)
    {
        GetText((int)Texts.StatusText).text = $"대기 취소: {reason}";
        Unsubscribe();
        ClosePopupUI();
    }

    private void OnClickCancel(PointerEventData evt)
    {
        Managers.Network.LeaveQueue();
        Unsubscribe();
        ClosePopupUI();
    }

    private void Unsubscribe()
    {
        Managers.Network.OnQueueStatus -= OnQueueStatus;
        Managers.Network.OnQueueAdmitted -= OnAdmitted;
        Managers.Network.OnQueueRejected -= OnRejected;
    }

    private void OnDestroy() => Unsubscribe();
}