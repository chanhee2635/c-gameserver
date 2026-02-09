using Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BaseController : MonoBehaviour
{
    public ulong ObjectId { get; set; }
    protected int TemplateId { get; set; }
    protected GameObjectType GameobjectType { get; set; }

    public void SetInfo(ulong objectId, int templateId, GameObjectType type)
    {
        ObjectId = objectId;
        TemplateId = templateId;
        GameobjectType = type;
    }
}
