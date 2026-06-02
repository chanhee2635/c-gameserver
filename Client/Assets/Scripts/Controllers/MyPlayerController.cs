using System;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.AI;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class MyPlayerController : PlayerController
{
    private NavMeshAgent _agent;
    private Camera _mainCam;
    private Image _attackCoolImage;

    private long _maxExp;
    private long _exp;

    private bool _isMoving;
    private bool _isSprinting;

    private const float SEND_INTERVAL_MOVE = 0.1f;   // moving / state change: 100ms
    private const float SEND_INTERVAL_IDLE = 0.5f;   // idle heartbeat: 500ms
    private Protocol.CreatureState _lastSendState;
    private float _sendTimer;
    private float _sendYaw;
    private const float RECONCILE_TOLERANCE_SQ = 1.0f;   // 1m: ignore minor server/client diff

    private int _maxCombo;
    private const float COMBO_INPUT_WINDOW = 0.45f;

    private bool  _isAttacking;
    private int   _currentCombo;
    private bool  _comboQueued;
    private float _attackTimer;
    private float _comboDuration;
    private Vector3 _sendVelocity;

    protected override void Awake()
    {
        base.Awake();
        InitComponents();
    }

    private void InitComponents()
    {
        _agent = GetComponent<NavMeshAgent>();
        if (_agent != null)
        {
            _agent.acceleration = 60f;
            _agent.stoppingDistance = 0.1f;
            _agent.updateRotation = false; // 직접 회전 제어 사용
        }
        _mainCam = Camera.main;
    }

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);

        _maxExp = Managers.Data.GetRequireExp(_templateId, Level + 1);
        _exp = info.StatInfo.Exp;
        _maxCombo = Managers.Data.GetMaxCombo(_templateId);

        if (_agent != null) _agent.speed = _baseSpeed;

        InitCamera(info.PosInfo.Yaw);
    }

    private void InitCamera(float yaw)
    {
        if (_mainCam == null) _mainCam = Camera.main;
        var cam = _mainCam.GetOrAddComponent<CameraController>();
        cam.MyPlayer = gameObject;
        cam.X = yaw;
        cam.Y = 20f;
        cam.SetPosition();
    }

    public void SetAttackCool(Image image)
    {
        _attackCoolImage = image;
        if (_attackCoolImage != null)
            _attackCoolImage.fillAmount = 1.0f;
    }
    public float GetExpRatio() => _maxExp > 0 ? (float)_exp / _maxExp : 1f;

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
            HandleInput();
        }

        CheckAndSendTickPacket();
    }


    private void HandleInput()
    {
        HandleSprinting();
        HandleMovementInput();

        if (Input.GetKeyDown(KeyCode.Space))
        {
            if (!IsUIFocused())
                TryAttack();
        }
    }

    private bool IsUIFocused()
        => EventSystem.current?.currentSelectedGameObject != null;

    private void HandleSprinting()
    {
        _isSprinting = Input.GetKey(KeyCode.LeftShift) && _isMoving;
        _agent.speed = _isSprinting ? _baseSpeed * 2.0f : _baseSpeed;
    }

    private void HandleMovementInput()
    {
        float h = Input.GetAxisRaw("Horizontal");
        float v = Input.GetAxisRaw("Vertical");

        Vector3 forward = Vector3.ProjectOnPlane(_mainCam.transform.forward, Vector3.up).normalized;
        Vector3 right = Vector3.ProjectOnPlane(_mainCam.transform.right, Vector3.up).normalized;
        Vector3 inputDir = (forward * v + right * h).normalized;

        if (inputDir != Vector3.zero)
        {
            _isMoving = true;
            _agent.Move(inputDir * _agent.speed * Time.deltaTime);
            _sendVelocity = inputDir * _agent.speed;

            Quaternion targetRot = Quaternion.LookRotation(inputDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, Time.deltaTime * 20f);

            _sendYaw = targetRot.eulerAngles.y;

            State = _isSprinting ? Protocol.CreatureState.Sprinting : Protocol.CreatureState.Moving;
        }
        else
        {
            if (_isMoving) StopMovement();
        }

        _agent.nextPosition = transform.position;

        float moveAnim = _isSprinting ? 1.0f
                       : _isMoving    ? 0.5f
                       : 0.0f;
        _animator.SetFloat("Move", moveAnim, 0.1f, Time.deltaTime);
    }

    private void StopMovement()
    {
        _isMoving = false;
        _isSprinting = false;
        _sendVelocity = Vector3.zero;
        _agent.velocity = Vector3.zero;
        State = Protocol.CreatureState.Idle;
        _animator.SetFloat("Move", 0f, 0.1f, Time.deltaTime);
    }

    public void TryAttack()
    {
        if (_isAttacking) return;

        StopMovement();
        StartComboAttack(1);
    }

    private void UpdateComboAttack()
    {
        _attackTimer += Time.deltaTime;

        // 콤보 큐 입력 (입력 창 이전)
        if (!_comboQueued && _currentCombo < _maxCombo)
        {
            if (_attackTimer >= _comboDuration * COMBO_INPUT_WINDOW)
            {
                if (Input.GetKeyDown(KeyCode.Space))
                    _comboQueued = true;
            }
        }

        // 애니메이션 시간 확인
        if (_attackTimer >= _comboDuration)
        {
            if (_comboQueued)
                StartComboAttack(_currentCombo + 1);
            else
                EndAttack();
        }
    }

    private void EndAttack()
    {
        _isAttacking = false;
        _currentCombo = 0;
        _comboQueued = false;
        State = Protocol.CreatureState.Idle;

        if (_attackCoolImage != null) _attackCoolImage.fillAmount = 1.0f;
    }

    private void StartComboAttack(int comboIndex)
    {
        _isAttacking = true;
        _comboQueued = false;
        _currentCombo = comboIndex;
        _attackTimer = 0f;
        _comboDuration = Managers.Data.GetComboDuration(_templateId, comboIndex);

        State = Protocol.CreatureState.Attack;

        _animator.SetFloat("Move", 0f);
        _animator.SetInteger("ComboIndex", comboIndex);
        _animator.SetTrigger("Attack");

        if (_attackCoolImage != null) _attackCoolImage.fillAmount = 0f;

        Managers.Network.Send(new Protocol.CAttack
        {
            Yaw = transform.eulerAngles.y,
            ComboIndex = comboIndex,
            Pos = Util.ToProto(transform.position)
        });
    }

    private void UpdateAttackCoolUI()
    {
        if (_attackCoolImage == null || _comboDuration <= 0f) return;
        _attackCoolImage.fillAmount = Mathf.Clamp01(_attackTimer / _comboDuration);
    }

    private void CheckAndSendTickPacket()
    {
        if (_isAttacking) return;

        bool moving = (State == Protocol.CreatureState.Moving || State == Protocol.CreatureState.Sprinting);

        if (_lastSendState != State)
        {
            SendMovePacket();
            return;
        }

        _sendTimer += Time.deltaTime;
        float interval = moving ? SEND_INTERVAL_MOVE : SEND_INTERVAL_IDLE;
        if (_sendTimer < interval) return;

        SendMovePacket();
    }

    private void SendMovePacket()
    {
        Managers.Network.Send(new Protocol.CMove
        {
            PosInfo = new Protocol.PosInfo
            {
                Pos = Util.ToProto(transform.position),
                Yaw = _sendYaw,
                State = State,
                Velocity = Util.ToProto(_sendVelocity)
            },
            SendServerTimeMs = Managers.Network.ServerNowMs()   // 0 until clock synced
        });

        _lastSendState = State;
        _sendTimer = 0f;
    }

    public void OnChangeExp(long exp)
    {
        _exp = exp;
        _maxExp = Managers.Data.GetRequireExp(_templateId, Level + 1);
    }

    public void OnLevelUp(int level, int maxHp, int hp, long exp)
    {
        _exp = exp;
        _maxExp = Managers.Data.GetRequireExp(_templateId, level + 1);
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

        _agent.Warp(pos);
        _animator.SetBool("IsDead", false);
        _animator.Play("Idle"); // 기본 상태 전환

        GameObject go = Managers.Resource.Instantiate("Effects/ReviveEffect");
        if (go != null) go.transform.position = pos + Vector3.up * 0.1f;
    }

    // Reconciliation: the server rejected our predicted move (anti-cheat / wall). Snap back
    // to the authoritative position. No input replay -- just roll back.
    public void OnMoveCorrection(Vector3 serverPos, float yaw)
    {
        if ((transform.position - serverPos).sqrMagnitude < RECONCILE_TOLERANCE_SQ)
            return;   // close enough: don't rubber-band on minor diffs

        _agent.Warp(serverPos);
        transform.rotation = Quaternion.Euler(0f, yaw, 0f);
        _sendYaw = yaw;
    }
}
