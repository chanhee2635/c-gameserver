using UnityEngine;

public class CreatureController : BaseController
{
    protected Animator _animator;
    protected int _templateId;
    protected float _baseSpeed;
    private bool _initialized;

    public Protocol.CreatureState State { get; protected set; } = Protocol.CreatureState.Idle;
    public string Name { get; protected set; }
    public int Level { get; protected set; }
    public int MaxHp { get; protected set; }
    public int Hp { get; protected set; }

    private Vector3 _destPos;
    private Quaternion _destDir;
    private float _noUpdateTimer;
    private const float NO_UPDATE_TIMEOUT = 0.35f;
    private const float SERVER_TICK_SEC = 0.1f;
    private LayerMask _groundMask;

    private Canvas _summaryCanvas;
    private UI_Summary _summaryUI;
    private bool _isUIShowing;
    private const float UI_SHOW_DIST_SQ = 200.0f;
    private const float UI_HIDE_DIST_SQ = 220.0f;

    protected virtual void Awake()
    {
        _animator = GetComponent<Animator>();
        _groundMask = LayerMask.GetMask("Ground");
    }

    private void OnDisable()
    {
        ClearUI();
    }

    protected virtual void Update()
    {
        if (!_initialized || State == Protocol.CreatureState.Dead) return;

        UpdatePositionAndRotation();
        UpdateTimeout();
        UpdateAnimation();
    }

    private void UpdatePositionAndRotation()
    {
        transform.rotation = Quaternion.Slerp(transform.rotation, _destDir, Time.deltaTime * 20f);

        Vector3 currentPos = transform.position;
        float distSq = (new Vector2(_destPos.x, _destPos.z) - new Vector2(currentPos.x, currentPos.z)).sqrMagnitude;

        if (distSq > 0.0001f)
        {
            float speed = (State == Protocol.CreatureState.Sprinting) ? _baseSpeed * 1.5f : _baseSpeed;
            Vector3 nextPos = Vector3.MoveTowards(currentPos, _destPos, speed * Time.deltaTime);

            float lerpSpeed = (nextPos.y < currentPos.y) ? 35f : 20f;
            nextPos.y = Mathf.Lerp(currentPos.y, _destPos.y, Time.deltaTime * lerpSpeed);

            transform.position = nextPos;
        }
        else if (Mathf.Abs(currentPos.y - _destPos.y) > 0.01f)
        {
            transform.position = new Vector3(currentPos.x, Mathf.Lerp(currentPos.y, _destPos.y, Time.deltaTime * 15f), currentPos.z);
        }
    }

    public void OnMoveUpdate(Protocol.PosInfo posInfo)
    {
        _noUpdateTimer = 0f;
        State = posInfo.State;
        _destDir = Quaternion.Euler(0, posInfo.Yaw, 0);

        Vector3 serverPos = new Vector3(posInfo.Pos.X, posInfo.Pos.Y, posInfo.Pos.Z);

        if (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting)
        {
            float speed = (State == Protocol.CreatureState.Sprinting) ? _baseSpeed * 1.5f : _baseSpeed;
            _destPos = serverPos + (_destDir * Vector3.forward * speed * SERVER_TICK_SEC);
        }
        else
        {
            _destPos = serverPos;
        }

        if (Physics.Raycast(_destPos + Vector3.up * 5f, Vector3.down, out RaycastHit hit, 10f, _groundMask))
            _destPos.y = hit.point.y;

        if ((transform.position - serverPos).sqrMagnitude > 25.0f)
            transform.position = serverPos;
    }

    public void UpdateUIByDistance(float distSq)
    {
        if (distSq > UI_HIDE_DIST_SQ)
        {
            if (_isUIShowing) ToggleUI(false);
            return;
        }

        if (distSq < UI_SHOW_DIST_SQ)
        {
            if (_summaryCanvas == null) SetupUI();
            if (!_isUIShowing) ToggleUI(true);
        }
    }

    private void SetupUI()
    {
        Transform uiRoot = transform.Find("UIRoot") ?? transform;
        GameObject go = Managers.Resource.Instantiate("UI/UI_Summary"); // 부모 없이 생성

        _summaryCanvas = go.GetComponent<Canvas>();
        _summaryUI = go.GetComponent<UI_Summary>();

        if (_summaryCanvas) _summaryCanvas.worldCamera = Camera.main;
        if (_summaryUI)
        {
            _summaryUI.Owner = this;
            _summaryUI.SetFollowTarget(uiRoot);
            _summaryUI.RefreshInfo();
        }
    }

    private void ToggleUI(bool show)
    {
        _isUIShowing = show;
        if (_summaryCanvas) _summaryCanvas.enabled = show;
        if (show) _summaryUI?.RefreshInfo();
    }

    private void ClearUI()
    {
        if (_summaryUI != null)
        {
            Managers.Resource.Destroy(_summaryUI.gameObject);
            _summaryUI = null;
            _summaryCanvas = null;
            _isUIShowing = false;
        }
    }

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);
        var summary = info.Summary;
        _templateId = summary.TemplateId;
        Name = summary.Name;
        Level = summary.Level;
        Hp = info.StatInfo.Hp;

        // Stat 정보 초기화 (Player/Monster 분기)
        InitStatByObjectType(summary.ObjectType);

        State = info.PosInfo.State;
        _destPos = position;
        _destDir = rotation;
        _noUpdateTimer = 0f;

        transform.SetPositionAndRotation(position, rotation);
        _initialized = true;
    }

    private void InitStatByObjectType(Protocol.GameObjectType type)
    {
        if (type == Protocol.GameObjectType.Player)
        {
            MaxHp = Managers.Data.GetMaxHp(_templateId, Level);
            _baseSpeed = Managers.Data.GetSpeed(_templateId, Level);
        }
        else if (type == Protocol.GameObjectType.Monster)
        {
            MaxHp = Managers.Data.GetMonsterMaxHp(_templateId);
            _baseSpeed = Managers.Data.GetMonsterSpeed(_templateId);
        }
    }

    public void OnAttack(Protocol.SAttack packet)
    {
        Vector3 serverPos = new Vector3(packet.Pos.X, packet.Pos.Y, packet.Pos.Z);
        if ((transform.position - serverPos).sqrMagnitude > 25f)
            transform.position = serverPos;

        _destPos = serverPos;
        _destDir = Quaternion.Euler(0f, packet.Yaw, 0f);

        _animator.SetInteger("ComboIndex", packet.ComboIndex);
        _animator.SetTrigger("Attack");
    }

    public void OnChangeHp(int hp, int damage)
    {
        Hp = hp;
        GameObject go = Managers.Resource.Instantiate("UI/UI_Damage");
        if (go != null)
        {
            go.transform.position = transform.position + Vector3.up * 1.4f;
            go.GetComponent<UI_Damage>()?.SetInfo(damage);
        }
        if (_isUIShowing) _summaryUI?.RefreshInfo();
    }

    public void OnDead()
    {
        if (State == Protocol.CreatureState.Dead) return;
        State = Protocol.CreatureState.Dead;
        _animator.SetBool("IsDead", true);
        _animator.SetFloat("Move", 0f);
        ClearUI(); // 죽으면 UI 제거
    }

    private void UpdateTimeout()
    {
        if (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting)
        {
            _noUpdateTimer += Time.deltaTime;
            if (_noUpdateTimer >= NO_UPDATE_TIMEOUT)
            {
                State = Protocol.CreatureState.Idle;
                _destPos = transform.position; // 현재 위치에 고정
            }
        }
    }

    private void UpdateAnimation()
    {
        if (!_animator) return;
        float moveAnim = State == Protocol.CreatureState.Sprinting ? 1.0f
                       : State == Protocol.CreatureState.Moving    ? 0.5f
                       : 0.0f;
        _animator.SetFloat("Move", moveAnim, 0.15f, Time.deltaTime);
    }

    public float GetHpRatio()
    {
        if (MaxHp <= 0) return 0f;
        return (float)Hp / MaxHp;
    }

    private void OnBecameVisible() { if (_animator) _animator.enabled = true; }
    private void OnBecameInvisible() { if (_animator) _animator.enabled = false; }
}
