using System.Collections.Generic;
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

    public int GetMaxHp(int templateId, int level) => GetPlayerData(templateId, level).maxHp;
    public int GetMaxMp(int templateId, int level) => GetPlayerData(templateId, level).maxMp;
    public float GetSpeed(int templateId, int level) => GetPlayerData(templateId, level).speed;
    public long GetRequireExp(int templateId, int level) => GetPlayerData(templateId, level).reqExp;
    public int GetMaxCombo(int templateId) => PrefabDataDict[templateId].maxCombo;

    public int GetMonsterMaxHp (int templateId) => MonsterDataDict[templateId].maxHp;
    public float GetMonsterSpeed (int templateId) => MonsterDataDict[templateId].speed;
    public float GetMonsterAttackDuration(int templateId) => GetComboDuration(templateId, 1);
    public float GetMonsterAttackHitDelay(int templateId) => GetComboHitDelay(templateId, 1) / 1000f;

    public float GetComboDuration(int templateId, int comboIndex)
    {
        if (!PrefabDataDict.TryGetValue(templateId, out PrefabData data)) return 0.5f;
        if (data.comboDurations == null || comboIndex >= data.comboDurations.Length) return 0.5f;
        return data.comboDurations[comboIndex];
    }

    public float GetComboHitDelay(int templateId, int comboIndex)
    {
        if (!PrefabDataDict.TryGetValue(templateId, out PrefabData data)) return 0.3f;
        if (data.comboHitDelays == null || comboIndex >= data.comboHitDelays.Length) return 0.3f;
        return data.comboHitDelays[comboIndex] / 1000f;  // ms → 초
    }

    PlayerData GetPlayerData(int templateId, int level)
    {
        if (PlayerDataDict.TryGetValue((templateId, level), out PlayerData data))
            return data;

        return null;
    }
    Loader LoadJson<Loader, Key, Value>(string path) where Loader : ILoader<Key, Value>
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");
        return Newtonsoft.Json.JsonConvert.DeserializeObject<Loader>(textAsset.text);
    }
}