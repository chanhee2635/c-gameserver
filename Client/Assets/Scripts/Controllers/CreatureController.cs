using UnityEngine;

public class CreatureController : BaseController
{
    protected Animator _animator;
    protected int      _templateId;
    protected float    _baseSpeed;
    private bool       _initialized;
    private Vector3    _velocity;
    private float      _lastMoveTime;

    // Projective velocity blending: the visual position advances at the server velocity every
    // frame (so it never stalls at a waypoint, even when the next packet is late), while the
    // positional error vs the latest server position is bled in smoothly. This decouples
    // visible speed (constant) from error correction, so arrival jitter no longer turns into
    // stop/overshoot - which is what made Sprinting (2x speed = 2x error) hitch.
    private Vector3    _renderPos;                      // continuous visual position
    private Vector3    _posError;                       // outstanding correction, bled in over time
    private const float CORRECT_RATE = 8f;              // 1/s; how fast positional error is absorbed

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
    private const float TELEPORT_DIST_SQ  = 100.0f;

    private Canvas     _summaryCanvas;
    private UI_Summary _summaryUI;
    private bool       _isUIShowing;
    private const float UI_SHOW_DIST_SQ = 200.0f;
    private const float UI_HIDE_DIST_SQ = 220.0f;

    protected virtual void Awake()
    {
        _animator = GetComponent<Animator>();

        // Position is driven explicitly (server-authoritative dead reckoning). Animator
        // root motion would also push the transform each frame, fighting the position
        // writes and making remote movement stutter. Disable it for replicated entities.
        if (_animator) _animator.applyRootMotion = false;
    }

    private void OnEnable()
    {
        if (_initialized)
        {
            transform.SetPositionAndRotation(_destPos, _destDir);
            _renderPos     = _destPos;
            _posError      = Vector3.zero;
            _velocity      = Vector3.zero;
            _lastMoveTime  = Time.time;
            _noUpdateTimer = 0f;
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

        transform.rotation = Quaternion.Slerp(transform.rotation, _destDir, dt * 20f);

        // 1) Keep moving at the server-reported velocity (continuous, never stalls).
        _renderPos += _velocity * dt;

        // 2) Bleed the outstanding positional error in smoothly (no snap, no backward jerk).
        float k = 1f - Mathf.Exp(-CORRECT_RATE * dt);
        _renderPos += _posError * k;
        _posError  *= (1f - k);

        transform.position = _renderPos;
    }

    public void OnMoveUpdate(Protocol.PosInfo posInfo)
    {
        _noUpdateTimer = 0f;
        State = posInfo.State;
        _destDir = Quaternion.Euler(0f, posInfo.Yaw, 0f);
        _destPos = new Vector3(posInfo.Pos.X, posInfo.Pos.Y, posInfo.Pos.Z);
        _velocity = new Vector3(posInfo.Velocity.X, posInfo.Velocity.Y, posInfo.Velocity.Z);
        _lastMoveTime = Time.time;

        if ((_destPos - _renderPos).sqrMagnitude > TELEPORT_DIST_SQ)
        {
            // Too far to blend: snap.
            _renderPos = _destPos;
            _posError  = Vector3.zero;
            transform.position = _renderPos;
        }
        else
        {
            // Correct toward the authoritative position; absorbed over ~1/CORRECT_RATE seconds.
            _posError = _destPos - _renderPos;
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
        _destPos       = position;
        _destDir       = rotation;
        _renderPos     = position;
        _posError      = Vector3.zero;
        _velocity      = Vector3.zero;
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
        Vector3 serverPos = new Vector3(packet.Pos.X, packet.Pos.Y, packet.Pos.Z);
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
            _animator.SetInteger("ComboIndex", packet.ComboIndex);
            _animator.SetTrigger("Attack");
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

    public void OnDead()
    {
        if (State == Protocol.CreatureState.Dead) return;
        State = Protocol.CreatureState.Dead;

        if (_animator)
        {
            _animator.SetBool("IsDead", true);
            _animator.SetFloat("Move", 0f);
        }
        ClearUI();
    }

    private void UpdateTimeout()
    {
        if (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting)
        {
            _noUpdateTimer += Time.deltaTime;
            if (_noUpdateTimer >= NO_UPDATE_TIMEOUT)
            {
                // No updates for a while: stop coasting so we don't drift off into the distance.
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
        _animator.SetFloat("Move", moveAnim, 0.15f, Time.deltaTime);
    }

    public float GetHpRatio()
    {
        if (MaxHp <= 0) return 0f;
        return (float)Hp / MaxHp;
    }

    private void OnBecameVisible()   { if (_animator) _animator.enabled = true;  }
    private void OnBecameInvisible() { if (_animator) _animator.enabled = false; }
}
