using Google.Protobuf.Collections;
using Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public interface ILoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
}

public class DataManager
{
    public Dictionary<int, PrefabData> PrefabDataDict { get; private set; } = new Dictionary<int, PrefabData>();
    public Dictionary<(int, int), PlayerData> PlayerDataDict { get; private set; } = new Dictionary<(int, int), PlayerData>();
    public Dictionary<int, MonsterData> MonsterDataDict { get; private set; } = new Dictionary<int, MonsterData>();

    public void Init()
    {
        PrefabDataDict = LoadJson<PrefabDataLoader, int, PrefabData>("PrefabData").MakeDict();
        PlayerDataDict = LoadJson<PlayerDataLoader, (int, int), PlayerData>("PlayerData").MakeDict();
        MonsterDataDict = LoadJson<MonsterDataLoader, int, MonsterData>("MonsterData").MakeDict();
    }

    public long GetTotalExpForLevel(int templateId, int level)
    {
        return PlayerDataDict[(templateId, level)].reqExp;
    }

    public long GetRequireExpForLevel(int templateId, int level)
    {
        long minExp = GetTotalExpForLevel(templateId, level);
        long maxExp = GetTotalExpForLevel(templateId, level + 1);
        return maxExp - minExp;
    }

    Loader LoadJson<Loader, Key, Value>(string path) where Loader : ILoader<Key, Value>
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");
        return Newtonsoft.Json.JsonConvert.DeserializeObject<Loader>(textAsset.text);
    }

    
}