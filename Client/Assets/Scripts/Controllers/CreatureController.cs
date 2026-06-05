using UnityEngine;

public class CreatureController : BaseController
{
    protected Animator _animator;
    protected int      _templateId;
    protected float    _baseSpeed;
    private bool       _initialized;

    private static readonly int MoveHash       = Animator.StringToHash("Move");
    private static readonly int ComboIndexHash = Animator.StringToHash("ComboIndex");
    private static readonly int AttackHash     = Animator.StringToHash("Attack");
    private static readonly int IsDeadHash     = Animator.StringToHash("IsDead");
    private Vector3    _velocity;
    private float      _lastMoveTime;

    private Vector3    _renderPos;                    
    private Vector3    _posError;                     
    private const float CORRECT_RATE = 8f;             
    private const float CORRECT_DEADZONE_SQ = 0.25f;   

    public Protocol.CreatureState State { get; protected set; } = Protocol.CreatureState.Idle;
    public string Name  { get; protected set; }
    public int    Level { get; protected set; }
    public int    MaxHp { get; protected set; }
    public int    Hp    { get; protected set; }

    private Vector3    _destPos;
    public Vector3     DestPos => _destPos;
    private Quaternion _destDir;
    private float      _noUpdateTimer;
    private const float NO_UPDATE_TIMEOUT = 1.0f;
    private const float TELEPORT_DIST_SQ  = 9.0f;

    private Canvas     _summaryCanvas;
    private UI_Summary _summaryUI;
    private bool       _isUIShowing;
    private const float UI_SHOW_DIST_SQ = 200.0f;
    private const float UI_HIDE_DIST_SQ = 220.0f;

    protected virtual void Awake()
    {
        _animator = GetComponent<Animator>();

        if (_animator)
            _animator.applyRootMotion = false;
    }

    private void OnEnable()
    {
        if (_initialized)
        {
            transform.SetPositionAndRotation(_destPos, _destDir);
            _renderPos     = _destPos;
            _posError      = Vector3.zero;
            _lastMoveTime  = Time.time;
            _noUpdateTimer = 0f;
            SyncDeathAnim();
        }
    }

    private void OnDisable()
    {
        if (_summaryCanvas != null) _summaryCanvas.enabled = false;
        _isUIShowing = false;
    }

    protected virtual void Update()
    {
        if (!_initialized || State == Protocol.CreatureState.Dead) return;

        UpdateMovement();
        UpdateTimeout();
        UpdateAnimation();
    }

    private void UpdateMovement()
    {
        float dt = Time.deltaTime;

        Quaternion rot = Quaternion.Slerp(transform.rotation, _destDir, dt * 20f);

        _renderPos += _velocity * dt;

        float k = 1f - Mathf.Exp(-CORRECT_RATE * dt);
        _renderPos += _posError * k;
        _posError  *= (1f - k);

        transform.SetPositionAndRotation(_renderPos, rot);
    }

    public void OnMoveUpdate(Protocol.PosInfo posInfo)
    {
        _noUpdateTimer = 0f;
        State = posInfo.State;
        SyncDeathAnim();
        _destDir  = Quaternion.Euler(0f, posInfo.Yaw, 0f);
        _destPos  = Util.ToUnity(posInfo.Pos);
        _velocity = Util.ToUnity(posInfo.Velocity);
        _lastMoveTime = Time.time;

        Vector3 error   = _destPos - _renderPos;
        float   errorSq = error.sqrMagnitude;

        if (errorSq > TELEPORT_DIST_SQ)
        {
            _renderPos = _destPos;
            _posError  = Vector3.zero;
            transform.position = _renderPos;
        }
        else
        {
            _posError = errorSq > CORRECT_DEADZONE_SQ ? error : Vector3.zero;
        }
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
        Transform  uiRoot = transform.Find("UIRoot") ?? transform;
        GameObject go     = Managers.Resource.Instantiate("UI/UI_Summary", transform);

        _summaryCanvas = go.GetComponent<Canvas>();
        _summaryUI     = go.GetComponent<UI_Summary>();

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
            if (Managers.Resource != null)
                Managers.Resource.Destroy(_summaryUI.gameObject);
            _summaryUI     = null;
            _summaryCanvas = null;
            _isUIShowing   = false;
        }
    }

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);

        foreach (var r in GetComponentsInChildren<Renderer>())
            r.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;

        var summary  = info.Summary;
        _templateId  = summary.TemplateId;
        Name         = summary.Name;
        Level        = summary.Level;
        Hp           = info.StatInfo.Hp;

        InitStatByObjectType(summary.ObjectType);

        State          = info.PosInfo.State;
        SyncDeathAnim();
        _destPos       = position;
        _destDir       = rotation;
        _renderPos     = position;
        _posError      = Vector3.zero;
        _velocity      = Util.ToUnity(info.PosInfo.Velocity);
        _lastMoveTime  = Time.time;
        _noUpdateTimer = 0f;

        transform.SetPositionAndRotation(position, rotation);
        _initialized = true;
    }

    private void InitStatByObjectType(Protocol.GameObjectType type)
    {
        if (type == Protocol.GameObjectType.Player)
        {
            MaxHp      = Managers.Data.GetMaxHp(_templateId, Level);
            _baseSpeed = Managers.Data.GetSpeed(_templateId, Level);
        }
        else if (type == Protocol.GameObjectType.Monster)
        {
            MaxHp      = Managers.Data.GetMonsterMaxHp(_templateId);
            _baseSpeed = Managers.Data.GetMonsterSpeed(_templateId);
        }
    }

    public void OnAttack(Protocol.SAttack packet)
    {
        Vector3 serverPos = Util.ToUnity(packet.Pos);
        if ((transform.position - serverPos).sqrMagnitude > TELEPORT_DIST_SQ)
            transform.position = serverPos;

        // Stop in place for the attack: clear motion/correction so nothing drifts.
        _renderPos = transform.position;
        _posError  = Vector3.zero;
        _velocity  = Vector3.zero;
        _destPos   = transform.position;
        _destDir   = Quaternion.Euler(0f, packet.Yaw, 0f);
        State = Protocol.CreatureState.Idle;

        if (_animator)
        {
            _animator.SetInteger(ComboIndexHash, packet.ComboIndex);
            _animator.SetTrigger(AttackHash);
        }
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
    protected void RefreshSummaryUI()
    {
        if (_isUIShowing) _summaryUI?.RefreshInfo();
    }

    public virtual void OnDead()
    {
        if (State == Protocol.CreatureState.Dead) return;
        State = Protocol.CreatureState.Dead;
        SyncDeathAnim();
        ClearUI();
    }
    protected void SyncDeathAnim()
    {
        if (_animator) _animator.SetBool(IsDeadHash, State == Protocol.CreatureState.Dead);
    }

    private void UpdateTimeout()
    {
        if (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting)
        {
            _noUpdateTimer += Time.deltaTime;
            if (_noUpdateTimer >= NO_UPDATE_TIMEOUT)
            {
                _velocity = Vector3.zero;
                _posError = Vector3.zero;
                _destPos  = _renderPos;
            }
        }
        else
        {
            _noUpdateTimer = 0f;
        }
    }

    private void UpdateAnimation()
    {
        if (!_animator) return;
        float moveAnim = State == Protocol.CreatureState.Sprinting ? 1.0f
                       : State == Protocol.CreatureState.Moving    ? 0.5f
                       : 0.0f;
        _animator.SetFloat(MoveHash, moveAnim, 0.15f, Time.deltaTime);
    }

    public float GetHpRatio()
    {
        if (MaxHp <= 0) return 0f;
        return (float)Hp / MaxHp;
    }

    private void OnBecameVisible()   { if (_animator) _animator.enabled = true;  }
    private void OnBecameInvisible() { if (_animator) _animator.enabled = false; }
}
