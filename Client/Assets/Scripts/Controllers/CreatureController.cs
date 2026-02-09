using Protocol;
using UnityEngine;

public class CreatureController : BaseController
{
    protected Animator _anim;
    public string Name { get; set; }
    public CreatureState State { get; set; }
    public int Level { get; set; }
    protected int MaxHp { get; set; }
    protected int Hp { get; set; }
    protected float Speed { get; set; }
    protected bool IsRun { get; set; } = false;
    protected Vector3 DestPos { get; set; }

    protected float RotateSpeed = 10f;
    int _groundMask;
    float _maxDistance = 5.0f;

    protected virtual void Awake()
    {
        _anim = GetComponent<Animator>();
        DestPos = transform.position;
        _groundMask = LayerMask.GetMask("Ground");
    }

    protected virtual void Update()
    {
        Movement();
    }

    protected virtual void Movement()
    {
        float moveValue = (State == CreatureState.Moving) ? (IsRun ? 1.0f : 0.5f) : 0.0f;
        _anim.SetFloat("Move", moveValue, 0.15f, Time.deltaTime);

        if (State != CreatureState.Moving)
            return;

        Vector3 currentPos = transform.position;
        Vector3 nextPos = Vector3.MoveTowards(currentPos, DestPos, Time.deltaTime * Speed * (IsRun ? 2.0f : 1.0f));
        
        RaycastHit hit;
        if (Physics.Raycast(nextPos + (Vector3.up * 5f), Vector3.down, out hit, 10.0f, _groundMask))
            nextPos.y = hit.point.y;

        transform.position = nextPos;

        Vector3 lookDir = DestPos - currentPos;
        lookDir.y = 0;
        if (lookDir.magnitude > 0.01f)
        {
            Quaternion targetRot = Quaternion.LookRotation(lookDir);
            transform.rotation = Quaternion.Slerp(transform.rotation, targetRot, Time.deltaTime * RotateSpeed);
        }
    }

    public virtual void SetInfo(ObjectInfo info)
    {
        base.SetInfo(info.Summary.ObjectId, info.Summary.TemplateId, info.Summary.ObjectType);
        State = info.PosInfo.State;
        Level = info.Summary.Level;
        Hp = info.StatInfo.Hp;
        transform.rotation = Quaternion.Euler(0f, info.PosInfo.Yaw, 0f);
        Vector3 startPos = new Vector3(info.PosInfo.X, info.PosInfo.Y, info.PosInfo.Z) + (Vector3.up * 5f);
        if (Physics.Raycast(startPos, Vector3.down, out RaycastHit hit, 10.0f, LayerMask.GetMask("Ground")))
            transform.position = hit.point;

    }

    public void SetPosInfo(PosInfo pos)
    {
        Vector3 newPos = new Vector3(pos.X, pos.Y, pos.Z);
        float distance = Vector3.Distance(transform.position, newPos);
        if (distance > _maxDistance)
            transform.position = newPos;

        State = pos.State;
        DestPos = new Vector3(pos.X, pos.Y, pos.Z);
        IsRun = pos.IsRun;
    }

    public void OnAttack(S_Attack packet)
    {
        Quaternion targetRotation = Quaternion.Euler(0f, packet.Yaw, 0f);
        transform.rotation = targetRotation;

        State = CreatureState.Attack;

        _anim.SetInteger("ComboIndex", packet.ComboIndex);
        _anim.SetTrigger("Attack");
    }

    public void OnChangeHp(S_ChangeHp packet)
    {
        Hp = packet.Hp;

        GameObject go = Managers.Resource.Instantiate("UI/UI_Damage");
        go.transform.position = transform.position + Vector3.up * 1.0f;
        UI_Damage ui = go.GetComponent<UI_Damage>();
        ui.SetInfo(packet.Damage);
    }

    public float GetHpRatio()
    {
        if (MaxHp <= 0) return 0;
        return Mathf.Clamp01((float)Hp / MaxHp);
    }

    public void OnDead()
    {
        State = CreatureState.Dead;

        _anim.SetBool("IsDead", true);
    }

    protected void SetUI()
    {
        Transform root = transform.Find("UIRoot");
        Transform parent = (root != null) ? root : transform;

        GameObject go = Managers.Resource.Instantiate("UI/UI_Summary", parent);
        UI_Summary summary = go.GetOrAddComponent<UI_Summary>();
        summary.owner = this; 
    }
}
