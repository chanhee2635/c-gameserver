using UnityEngine;

public class CreatureController : BaseController
{
    protected Animator _animator;

    public Protocol.CreatureState State { get; protected set; } = Protocol.CreatureState.Idle;
    public int Level { get; protected set; }
    public int MaxHp { get; protected set; }
    public int Hp { get; protected set; }
    public string Name { get; protected set; }
    protected int _templateId;
    protected float _baseSpeed;

    private Vector3 _destPos;
    private Quaternion _destDir;
    private LayerMask _groundMask;

    private float _noUpdateTimer = 0f;
    private const float NO_UPDATE_TIMEOUT = 0.35f;
    private const float SERVER_TICK_SEC = 0.1f;
    private bool _initialized = false;

    protected virtual void Awake()
    {
        _animator = GetComponent<Animator>();
        _groundMask = LayerMask.GetMask("Ground");
    }

    private void OnEnable()
    {
        if (!_initialized) return;

        Vector3 snapped = _destPos;
        if (Physics.Raycast(snapped + Vector3.up * 20f, Vector3.down, out RaycastHit hit, 40f, _groundMask))
            snapped.y = hit.point.y;

        transform.position = snapped;
        transform.rotation = _destDir;
    }

    protected virtual void Update()
    {
        if (State == Protocol.CreatureState.Dead) return;

        float moveSpeed = (State == Protocol.CreatureState.Sprinting) ? _baseSpeed * 1.5f : _baseSpeed;
        Vector3 nextPos = Vector3.MoveTowards(transform.position, _destPos, moveSpeed * Time.deltaTime);

        if (Physics.Raycast(nextPos + Vector3.up * 20f, Vector3.down, out RaycastHit hit, 40f, _groundMask))
            nextPos.y = hit.point.y;

        transform.position = nextPos;
        transform.rotation = Quaternion.Slerp(transform.rotation, _destDir, Time.deltaTime * 10f);

        if (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting)
        {
            _noUpdateTimer += Time.deltaTime;
            if (_noUpdateTimer >= NO_UPDATE_TIMEOUT)
            {
                State = Protocol.CreatureState.Idle;
                _noUpdateTimer = 0f;
            }
        }
        else
        {
            _noUpdateTimer = 0f;
        }

        float moveAnim = (State == Protocol.CreatureState.Sprinting) ? 1.0f
                       : (State == Protocol.CreatureState.Moving) ? 0.5f : 0.0f;
        _animator.SetFloat("Move", moveAnim, 0.15f, Time.deltaTime);
    }

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);

        _templateId = info.Summary.TemplateId;
        Name = info.Summary.Name;
        Level = info.Summary.Level;
        Hp = info.StatInfo.Hp;

        if (_objectType == Protocol.GameObjectType.Player)
        {
            MaxHp = Managers.Data.GetMaxHp(_templateId, Level);
            _baseSpeed = Managers.Data.GetSpeed(_templateId, Level);
        }
        else if (_objectType == Protocol.GameObjectType.Monster)
        {
            MaxHp = Managers.Data.GetMonsterMaxHp(_templateId);
            _baseSpeed = Managers.Data.GetMonsterSpeed(_templateId);
        }

        State = info.PosInfo.State;
        _destPos = position;
        _destDir = rotation;
        _noUpdateTimer = 0f;

        transform.position = position;
        transform.rotation = rotation;

        _initialized = true;
    }

    public void OnMoveUpdate(Protocol.PosInfo posInfo)
    {
        Vector3 serverPos = new Vector3(posInfo.Pos.X, posInfo.Pos.Y, posInfo.Pos.Z);
        Quaternion serverRot = Quaternion.Euler(0, posInfo.Yaw, 0);

        _noUpdateTimer = 0f;
        State = posInfo.State;
        _destDir = serverRot;

        if (posInfo.State == Protocol.CreatureState.Moving ||
             posInfo.State == Protocol.CreatureState.Sprinting)
        {
            float speed = (posInfo.State == Protocol.CreatureState.Sprinting)
                ? _baseSpeed * 1.5f : _baseSpeed;
            Vector3 dir = serverRot * Vector3.forward;
            _destPos = serverPos + dir * speed * SERVER_TICK_SEC;
        }
        else
        {
            _destPos = serverPos;
        }


        // 5m 이상 오차면 즉시 워프 (텔레포트, 씬 전환)
        if (Vector3.Distance(transform.position, serverPos) > 5f)
        {
            transform.position = serverPos;
            transform.rotation = serverRot;
        }
    }

    public float GetHpRatio() => MaxHp > 0 ? (float)Hp / MaxHp : 0f;

    protected void SetUI()
    {
        Transform root = transform.Find("UIRoot");
        Transform parent = (root != null) ? root : transform;

        GameObject go = Managers.Resource.Instantiate("UI/UI_Summary", parent);
        UI_Summary summary = go.GetOrAddComponent<UI_Summary>();
        summary.owner = this;
    }

    public void OnAttack(Protocol.S_Attack packet)
    {
        Vector3 serverPos = new Vector3(packet.Pos.X, packet.Pos.Y, packet.Pos.Z);
        float dist = Vector3.Distance(transform.position, serverPos);

        if (dist >= 5f)
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
        if (go == null) return;

        go.transform.position = transform.position + Vector3.up * 1.4f;

        UI_Damage ui = go.GetComponent<UI_Damage>();
        ui.SetInfo(damage);
    }

    public void OnDead()
    {
        if (State == Protocol.CreatureState.Dead) return;

        State = Protocol.CreatureState.Dead;
        _animator.SetBool("IsDead", true);
        _animator.SetFloat("Move", 0f);
    }

    public virtual void OnLevelUp(int level, int maxHp, int hp)
    {
        Level = level;
        MaxHp = maxHp;
        Hp = hp;

        GameObject go = Managers.Resource.Instantiate("Effects/LevelUpEffect");
        if (go == null) return;
        go.transform.position = transform.position + Vector3.up;
    }
}