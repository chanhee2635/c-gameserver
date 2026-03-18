using System.Collections;
using System.Collections.Generic;

public class Define
{
    public enum Scene
    {
        Unknown,
        Login,
        SelectPlayer,
        Game
    }

    public enum Sound
    {
        Bgm,
        Effect,
        MaxCount,
    }

    public enum UIEvent
    {
        Click,
        Drag,
    }

    public enum CreatureState
    {
        Idle = 0,
        Moving = 1, 
        Sprinting = 2,
        Attack = 3,
        Dead = 4
    }

    public enum ObjectType
    {
        Player,
        Monster
    }
}
