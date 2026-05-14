using UnityEngine;

public class BaseController : MonoBehaviour
{
    public ulong                   ObjectId   { get; protected set; }
    public Protocol.GameObjectType ObjectType { get; protected set; }

    public virtual void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        ObjectId   = info.Summary.ObjectId;
        ObjectType = info.Summary.ObjectType;
    }
}
