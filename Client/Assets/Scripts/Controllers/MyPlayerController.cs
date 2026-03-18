using System;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.AI;
using UnityEngine.UI;

public class MyPlayerController : PlayerController
{
    NavMeshAgent _agent;
    Image _attackCoolImage;

    long _maxExp;
    long _exp;

    private bool    _isMoving = false;
    private bool    _isSprinting = false;

    private float       _tickTimer = 0f;
    private const float TICK_INTERVAL = 0.1f;
    private const float FORCE_SYNC_INTERVAL = 0.5f;
    private float       _lastSendTime;
    private Protocol.CreatureState _lastSendState;

    private Camera _mainCam;

    private int _maxCombo = 0;
    private const float COMBO_INPUT_WINDOW = 0.45f;

    private bool _isAttacking = false;   // 공격 모션 진행 중
    private int _currentCombo = 0;       // 현재 실행 콤보 인덱스 (0 = 없음)
    private bool _comboQueued = false;   // 다음 콤보 예약 여부
    private float _attackTimer = 0f;      // 현재 모션 경과 시간
    private float _comboDuration = 0f;      // 현재 모션의 총 길이

    protected override void Awake()
    {
        base.Awake();
        _agent = GetComponent<NavMeshAgent>();
        _agent.acceleration = 60f;
        _agent.stoppingDistance = 0.1f;
    }

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);
        _maxExp = Managers.Data.GetRequireExp(_templateId, Level);
        _exp = info.StatInfo.Exp;
        _agent.speed = _baseSpeed;

        _maxCombo = Managers.Data.GetMaxCombo(_templateId);

        CameraController cam = Camera.main.GetOrAddComponent<CameraController>();
        cam.MyPlayer = gameObject;
        cam.X = info.PosInfo.Yaw;
        cam.Y = 0f;

        _mainCam = Camera.main;
    }

    public void SetAttackCool(Image image) { _attackCoolImage = image; }
    public float GetExpRatio() => _maxExp > 0 ? (float)_exp / _maxExp : 0f;

    protected override void Update()
    {
        if (State == Protocol.CreatureState.Dead) return;

        if (_isAttacking)
        {
            UpdateComboAttack();
            UpdateAttackCoolUI();
        }
        else
        {
            HandleSprinting();
            HandleMovementInput();
            if (Input.GetKeyDown(KeyCode.Space))
                TryAttack();
        }

        CheckAndSendTickPacket();
    }
    private void HandleSprinting()
    {
        _isSprinting = Input.GetKey(KeyCode.LeftShift) && _isMoving;
        _agent.speed = _isSprinting ? _baseSpeed * 1.5f : _baseSpeed;
    }

    private void HandleMovementInput()
    {
        float h = Input.GetAxisRaw("Horizontal");
        float v = Input.GetAxisRaw("Vertical");

        if (_mainCam == null) _mainCam = Camera.main;   
        Transform cam = _mainCam.transform;
        Vector3 forward = Vector3.ProjectOnPlane(cam.forward, Vector3.up).normalized;
        Vector3 right = Vector3.ProjectOnPlane(cam.right, Vector3.up).normalized;
        Vector3 inputDir = (forward * v + right * h).normalized;

        if (inputDir != Vector3.zero)
        {
            _isMoving = true;
            _agent.Move(inputDir * _agent.speed * Time.deltaTime);
            transform.rotation = Quaternion.Slerp(transform.rotation, Quaternion.LookRotation(inputDir), Time.deltaTime * 10f);
            State = _isSprinting ? Protocol.CreatureState.Sprinting : Protocol.CreatureState.Moving;
        }
        else
        {
            _isMoving = false;
            _agent.velocity = Vector3.zero;
            State = Protocol.CreatureState.Idle;
        }

        float moveAnim = (State == Protocol.CreatureState.Sprinting) ? 1.0f : (State == Protocol.CreatureState.Moving) ? 0.5f : 0.0f;
        _animator.SetFloat("Move", moveAnim, 0.15f, Time.deltaTime);
    }

    public void TryAttack()
    {
        if (_isAttacking) return;
        StartComboAttack(1);
    }

    private void UpdateComboAttack()
    {
        _attackTimer += Time.deltaTime;

        bool inInputWindow = _attackTimer >= _comboDuration * COMBO_INPUT_WINDOW;
        if (inInputWindow && !_comboQueued && _currentCombo < _maxCombo)
        {
            if (Input.GetKeyDown(KeyCode.Space))
                _comboQueued = true;
        }

        if (_attackTimer >= _comboDuration)
        {
            if (_comboQueued)
                StartComboAttack(_currentCombo + 1);
            else
            {
                _isAttacking = false;
                _currentCombo = 0;
                State = Protocol.CreatureState.Idle;
                _animator.SetFloat("Move", 0f);

                if (_attackCoolImage != null)
                    _attackCoolImage.fillAmount = 0f;
            }
        }
    }

    private void UpdateAttackCoolUI()
    {
        if (_attackCoolImage == null) return;
        _attackCoolImage.fillAmount = _comboDuration > 0f ? _attackTimer / _comboDuration : 0f;
    }

    private void StartComboAttack(int comboIndex)
    {
        _isAttacking = true;
        _comboQueued = false;
        _currentCombo = comboIndex;
        _attackTimer = 0f;
        _comboDuration = Managers.Data.GetComboDuration(_templateId, comboIndex);

        _agent.velocity = Vector3.zero;
        State = Protocol.CreatureState.Attack;

        _animator.SetFloat("Move", 0f);
        _animator.SetInteger("ComboIndex", comboIndex);
        _animator.SetTrigger("Attack");

        if (_attackCoolImage != null)
            _attackCoolImage.fillAmount = 0f;

        Protocol.C_Attack packet = new Protocol.C_Attack
        {
            Yaw = transform.rotation.eulerAngles.y,
            ComboIndex = comboIndex,
            Pos = Util.ToProto(transform.position)
        };
        Managers.Network.Send(packet);
    }

    private void CheckAndSendTickPacket()
    {
        if (_isAttacking) return;

        _tickTimer += Time.deltaTime;
        if (_tickTimer < TICK_INTERVAL) return;
        _tickTimer = 0f;

        bool isMovingNow = (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting);
        bool stateChanged = _lastSendState != State;
        bool isTimeOut = Time.time - _lastSendTime >= FORCE_SYNC_INTERVAL;

        if (isMovingNow || stateChanged || isTimeOut)
            SendMovePacket();
    }

    private void SendMovePacket()
    {
        Protocol.C_Move packet = new Protocol.C_Move
        {
            PosInfo = new Protocol.PosInfo
            {
                Pos = Util.ToProto(transform.position),
                Yaw = transform.rotation.eulerAngles.y,
                State = State
            }
        };

        Managers.Network.Send(packet);

        _lastSendState = State;
        _lastSendTime = Time.time;
    }

    public void OnChangeExp(long exp)
    {
        _exp = exp;
        _maxExp = Managers.Data.GetRequireExp(_templateId, Level);
    }

    public void OnLevelUp(int level, int maxHp, int hp, long exp)
    {
        _exp = exp;
        _maxExp = Managers.Data.GetRequireExp(_templateId, level);
        base.OnLevelUp(level, maxHp, hp);
    }

    public void OnRevive(Vector3 pos, int hp, int maxHp)
    {
        Hp = hp;
        MaxHp = maxHp;
        State = Protocol.CreatureState.Idle;

        _isAttacking = false;
        _comboQueued = false;
        _currentCombo = 0;

        transform.position = pos;
        _agent.Warp(pos);

        _animator.SetBool("IsDead", false);
        _animator.SetFloat("Move", 0f);

        GameObject go = Managers.Resource.Instantiate("Effects/ReviveEffect");
        if (go != null)
            go.transform.position = pos + Vector3.up * 0.1f;
    }

    /*void UpdateAttackCoolUI()
    {
        if (_attackCoolImage == null) return;

        float totalCoolTime = _currentAttackDuration;
        float elapsedTime = Time.time - _lastAttackTime;

        if (elapsedTime < totalCoolTime)
        {
            float remainTime = totalCoolTime - elapsedTime;
            _attackCoolImage.fillAmount = remainTime / totalCoolTime;
        }
        else
        {
            _attackCoolImage.fillAmount = 0;

            if (State == CreatureState.Attack)
            {
                if (_reserveNextCombo)
                    ExecuteAttack();
                else
                    State = CreatureState.Idle;
            }
        }
    }

    bool IsMoveTick()
    {
        return *//*State == CreatureState.Moving && *//*Time.time - _lastSendTime > SEND_INTERVAL;
    }

    bool IsStateChanged()
    {
        return LastState != State || LastRunState != IsRun;
    }

    bool IsAttack()
    {
        return State == CreatureState.Attack || _reserveNextCombo == true;
    }

    float _currentAttackDuration = 1.0f;

    private void GetInput()
    {
        if (IsAttack())
        {
            if (Input.GetKeyDown(KeyCode.Space))
            {
                HandleAttackInput();
            }
            return;
        }

        if (Input.GetKeyDown(KeyCode.Space))
        {
            HandleAttackInput();

            if (IsAttack()) return;
        }

        IsRun = Input.GetKey(KeyCode.LeftShift);
        float v = Input.GetAxisRaw("Vertical");
        float h = Input.GetAxisRaw("Horizontal");

        if (v != 0 || h != 0)
        {
            State = CreatureState.Moving;
            Transform cam = Camera.main.transform;
            Vector3 forward = cam.forward * v;
            Vector3 right = cam.right * h;
            Vector3 moveDir = (forward + right).normalized;
            float speed = moveSpeed * (IsRun ? 2.0f : 1.0f);
            DestPos = transform.position + (moveDir * speed * Time.deltaTime);
        }
        else
        {
            State = CreatureState.Idle;
            DestPos = transform.position;
        }
    }

    void HandleAttackInput()
    {
        if (Time.time < _lastAttackTime + (_currentAttackDuration * 0.8f))
            return;

        if (Time.time - _lastAttackTime > ComboTimeout)
        {
            _currentComboIndex = 0;
            _reserveNextCombo = false;
        }

        if (_reserveNextCombo) return;

        if (State == CreatureState.Attack)
        {
            _reserveNextCombo = true;
            _nextComboIndex = (_currentComboIndex % MaxCombo) + 1;
            return;
        }

        _nextComboIndex = (_currentComboIndex % MaxCombo) + 1;
        ExecuteAttack();
    }

    protected void MyMovement()
    {
        if (IsAttack())
        {
            _anim.SetFloat("Move", 0);
            return;
        }

        float moveValue = (State == CreatureState.Moving) ? (IsRun ? 1.0f : 0.5f) : 0.0f;
        _anim.SetFloat("Move", moveValue, 0.15f, Time.deltaTime);

        // destPos 의 x, z 값을 가지고 y를 찾는다.
        Vector3 currentPos = transform.position;
        Vector3 dir = DestPos - currentPos;
        dir.y = 0;

        if (dir.magnitude > 0.01f)
        {
            Quaternion targetRot = Quaternion.LookRotation(dir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, Time.deltaTime * RotateSpeed);

            Vector3 movement = dir.normalized * Time.deltaTime * moveSpeed * (IsRun ? 2.0f : 1.0f);

            if (movement.magnitude > dir.magnitude)
                movement = dir;

            agent.Move(movement);
        }
    }

    public override void SetInfo(ObjectInfo info)
    {
        base.SetInfo(info);

        CameraController cam = Camera.main.GetComponent<CameraController>();
        cam.MyPlayer = gameObject;
        cam.X = info.PosInfo.Yaw;
        cam.Y = 0f;

        agent = gameObject.GetOrAddComponent<NavMeshAgent>();
        if (agent != null) agent.enabled = false;

        Vector3 serverPos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        NavMeshHit hit;
        if (NavMesh.SamplePosition(serverPos + (Vector3.up * 5f), out hit, 10.0f, NavMesh.AllAreas))
        {
            agent.Warp(hit.position);
            agent.enabled = true;
        }

        PlayerData data = Managers.Data.PlayerDataDict[(TemplateId, Level)];
        MaxMp = data.maxMp;
        AttackSpeed = data.attackSpeed;
        MaxCombo = Managers.Data.PrefabDataDict[TemplateId].maxCombo;
        Mp = info.StatInfo.Mp;
        TotalExp = Managers.Data.GetRequireExpForLevel(TemplateId, Level);
        Exp = info.StatInfo.Exp;
    }

    void SendMovePacket()
    {
        C_Move packet = new C_Move();
        PosInfo info = new PosInfo
        {
            State = State,
            X = transform.position.x,
            Y = transform.position.y,
            Z = transform.position.z,
            Yaw = transform.rotation.eulerAngles.y,
            IsRun = IsRun
        };
        packet.Pos = info;
        Managers.Network.Send(packet);
    }

    public void OnChangeStat(int level)
    {
        Level = level;
        PlayerData data = Managers.Data.PlayerDataDict[(TemplateId, Level)];
        MaxHp = data.maxHp;
        MaxMp = data.maxMp;
        TotalExp = Managers.Data.GetRequireExpForLevel(TemplateId, level);
        Hp = MaxHp;
        Mp = MaxMp;
    }

    public void OnChangeExp(int level, long exp)
    {
        Exp = exp;
        TotalExp = Managers.Data.GetRequireExpForLevel(TemplateId, level);

        // 레벨이 변경됐다면
        if (Level != level)
        {
            Level = level;
            PlayerData data = Managers.Data.PlayerDataDict[(TemplateId, Level)];
            MaxHp = data.maxHp;
            MaxMp = data.maxMp;
            Hp = MaxHp;
            Mp = MaxMp;
        }
    }

    public float GetMpRatio()
    {
        if (MaxMp <= 0) return 0;
        return Mathf.Clamp01((float)Mp / MaxMp);
    }

    public float GetExpRatio()
    {
        long levelExp = Managers.Data.GetTotalExpForLevel(TemplateId, Level);
        long curExp = Exp - levelExp;

        if (TotalExp <= 0) return 0;
        return Mathf.Clamp01((float)curExp / TotalExp);
    }

    private void ExecuteAttack()
    {
        if (State == CreatureState.Dead) return;

        State = CreatureState.Attack;
        _reserveNextCombo = false;
        _currentComboIndex = _nextComboIndex;
        _lastAttackTime = Time.time;

        // 패킷 전송
        C_Attack packet = new C_Attack();
        packet.Yaw = transform.rotation.eulerAngles.y;
        packet.ComboIndex = _currentComboIndex;
        Managers.Network.Send(packet);

        if (_attackDurations.TryGetValue(_currentComboIndex, out float length))
            _currentAttackDuration = length / AttackSpeed;

        _anim.speed = AttackSpeed;
        _anim.SetInteger("ComboIndex", _currentComboIndex);
        _anim.SetTrigger("Attack");
    }*/
}
