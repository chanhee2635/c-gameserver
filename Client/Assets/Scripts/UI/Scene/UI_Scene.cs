using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class UI_Scene : UI_Base
{
    protected override void Init()
    {
        Managers.UI.SetCanvas(gameObject, false);
    }
}
