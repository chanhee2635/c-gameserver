using UnityEngine;

public class BaseController : MonoBehaviour
{
    protected ulong                   _objectId;
    protected Protocol.GameObjectType _objectType;

    public ulong                   GetObjectId() => _objectId;
    public Protocol.GameObjectType GetObjectType() => _objectType;

    public virtual void SetInfo(Protocol.ObjectInfo info, Vector3 position, Quaternion rotation)
    {
        _objectId   = info.Summary.ObjectId;
        _objectType = info.Summary.ObjectType;
    }
}
