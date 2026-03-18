using UnityEngine;

public class PlayerController : CreatureController
{
    protected int _mp;
    protected int _maxMp;

    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);

        _mp = info.StatInfo.Mp;
        _maxMp = Managers.Data.GetMaxMp(_templateId, Level);

        SetUI();
    }

    public float GetMpRatio() => _maxMp > 0 ? (float)_mp / _maxMp : 0f;

    /*public void SetLevel(int level)
    {
        Level = level;
        PlayerData data = Managers.Data.PlayerDataDict[(TemplateId, Level)];
        MaxHp = data.maxHp;
        Hp = MaxHp;
    }

    public void SetRevive(PosInfo posInfo, int hp)
    {
        SetPosInfo(posInfo);
        Hp = hp;
        _anim.SetBool("IsDead", false);
    }*/
}
