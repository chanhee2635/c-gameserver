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
    }

    public float GetMpRatio() => _maxMp > 0 ? (float)_mp / _maxMp : 0f;

    public virtual void OnLevelUp(int level, int maxHp, int hp)
    {
        Level = level;
        MaxHp = maxHp;
        Hp = hp;
        _maxMp = Managers.Data.GetMaxMp(_templateId, level);
        _mp = _maxMp;
    }
}
