using Protocol;
using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting.FullSerializer;
using UnityEngine;

public class MonsterController : CreatureController
{
    public override void SetInfo(ObjectInfo info)
    {
        base.SetInfo(info);

        MonsterData data = Managers.Data.MonsterDataDict[TemplateId];
        MaxHp = data.maxHp;
        Speed = data.speed;
        Name = Managers.Data.PrefabDataDict[TemplateId].name;

        SetUI();
    }
}
