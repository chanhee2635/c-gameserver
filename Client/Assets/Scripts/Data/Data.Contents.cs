using System;
using System.Collections.Generic;
using UnityEngine;

#region PrefabData
[Serializable]
public class PrefabData
{
    public int id;
    public string name;
    public string prefabPath;
    public int maxCombo;
}

[Serializable]
public class PrefabDataLoader : ILoader<int, PrefabData>
{
    public List<PrefabData> data = new List<PrefabData>();

    public Dictionary<int, PrefabData> MakeDict()
    {
        Dictionary<int, PrefabData> dict = new Dictionary<int, PrefabData>();
        foreach (PrefabData d in data)
            dict.Add(d.id, d);
        return dict;
    }
}
#endregion

#region PlayerData
[Serializable]
public class PlayerData
{
    public int id;
    public int level;
    public int maxHp;
    public int maxMp;
    public long reqExp;
    public int attack;
    public int defense;
    public float speed;
    public float attackRange;
    public float attackSpeed;
    public float attackAngle;
}

[Serializable]
public class PlayerDataLoader : ILoader<(int, int), PlayerData>
{
    public List<PlayerData> data = new List<PlayerData>();

    public Dictionary<(int, int), PlayerData> MakeDict()
    {
        Dictionary<(int, int), PlayerData> dict = new Dictionary<(int, int), PlayerData>();
        foreach (PlayerData d in data)
            dict.Add((d.id, d.level), d);
        return dict;
    }
}
#endregion

#region MonsterData
[Serializable]
public class MonsterData
{
    public int id;
    public int level;
    public int maxHp;
    public int maxMp;
    public long reqExp;
    public int attack;
    public int defense;
    public float speed;
    public float attackRange;
    public float attackSpeed;
    public float attackAngle;
}

[Serializable]
public class MonsterDataLoader : ILoader<int, MonsterData>
{
    public List<MonsterData> data = new List<MonsterData>();

    public Dictionary<int, MonsterData> MakeDict()
    {
        Dictionary<int, MonsterData> dict = new Dictionary<int, MonsterData>();
        foreach (MonsterData d in data)
            dict.Add(d.id, d);
        return dict;
    }
}
#endregion