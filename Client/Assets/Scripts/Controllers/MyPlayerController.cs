using Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;
using UnityEngine.UI;

public class MyPlayerController : PlayerController
{
    NavMeshAgent _navMeshAgent;
    Image _attackCoolImage;
    protected int MaxMp { get; set; }
    protected int Mp { get; set; }
    protected long Exp { get; set; }
    protected long TotalExp { get; set; }
    protected int MaxCombo { get; set; }
    protected float AttackSpeed { get; set; }

    float _lastSendTime = 0;
    const float SEND_INTERVAL = 0.1f;

    CreatureState LastState { get; set; } = CreatureState.Idle;

    bool LastRunState { get; set; } = false;

    int _currentComboIndex = 0;
    int _nextComboIndex = 0;
    float _lastAttackTime = 0;
    float ComboTimeout = 2.0f;
    bool _reserveNextCombo = false;

    Dictionary<int, float> _attackDurations = new Dictionary<int, float>();

    protected void Start()
    {
        _navMeshAgent = GetComponent<NavMeshAgent>();
        _navMeshAgent.updateRotation = false;
        _navMeshAgent.updatePosition = true;
        _navMeshAgent.acceleration = 100f;

        // TODO: 데이터로 서버와 공유
        _attackDurations[1] = GetClipLength("Attack_1");
        _attackDurations[2] = GetClipLength("Attack_2");
        _attackDurations[3] = GetClipLength("Attack_3") + GetClipLength("Attack_4");
    }

    float GetClipLength(string clipName)
    {
        foreach(var clip in _anim.runtimeAnimatorController.animationClips)
        {
            if (clip.name == clipName) return clip.length;
        }

        return 0f;
    }

    public void SetAttackCool(Image attackCool)
    {
        _attackCoolImage = attackCool;
    }

    protected override void Update()
    {
        if (State == CreatureState.Dead) return;

        GetInput();

        MyMovement();

        if (!IsAttack() && (IsStateChanged() || IsMoveTick()))
        {
            SendMovePacket();
            LastState = State;
            LastRunState = IsRun;
            _lastSendTime = Time.time;
        }

        UpdateAttackCoolUI();
    }

    void UpdateAttackCoolUI()
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
        return State == CreatureState.Moving && Time.time - _lastSendTime > SEND_INTERVAL;
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
            float speed = Speed * (IsRun ? 2.0f : 1.0f);
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

            Vector3 movement = dir.normalized * Time.deltaTime * Speed * (IsRun ? 2.0f : 1.0f);

            if (movement.magnitude > dir.magnitude)
                movement = dir;

            _navMeshAgent.Move(movement);
        }
    }

    public override void SetInfo(ObjectInfo info)
    {
        base.SetInfo(info);

        CameraController cam = Camera.main.GetComponent<CameraController>();
        cam.MyPlayer = gameObject;
        cam.X = info.PosInfo.Yaw;
        cam.Y = 0f;

        _navMeshAgent = gameObject.GetOrAddComponent<NavMeshAgent>();
        if (_navMeshAgent != null) _navMeshAgent.enabled = false;

        Vector3 serverPos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z);

        NavMeshHit hit;
        if (NavMesh.SamplePosition(serverPos + (Vector3.up * 5f), out hit, 10.0f, NavMesh.AllAreas))
        {
            _navMeshAgent.Warp(hit.position);
            _navMeshAgent.enabled = true;
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
    }
}
