using UnityEngine;

public class MonsterController : CreatureController
{
    public override void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        base.SetInfo(info, position, rotation);

        SetUI();
    }
}
