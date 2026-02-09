using Protocol;
using System;

public class PlayerController : CreatureController
{
    public override void SetInfo(ObjectInfo info)
    {
        base.SetInfo(info);

        PlayerData data = Managers.Data.PlayerDataDict[(TemplateId, Level)];
        MaxHp = data.maxHp;
        Speed = data.speed;
        Name = info.Summary.Name;

        SetUI();
    }

    public void SetLevel(int level)
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
    }
}
