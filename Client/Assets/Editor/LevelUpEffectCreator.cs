// Assets/Editor/LevelUpEffectCreator.cs
// Tools > Create LevelUpEffect Prefab

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using TMPro;

public static class LevelUpEffectCreator
{
    [MenuItem("Tools/Create LevelUpEffect Prefab")]
    public static void Create()
    {
        GameObject root = new GameObject("LevelUpEffect");
        root.AddComponent<Poolable>();
        LevelUpEffect effect = root.AddComponent<LevelUpEffect>();

        ParticleSystem rise = CreateRiseParticle(root);   // 바닥 빛기둥
        ParticleSystem glow = CreateGlowParticle(root);   // 주위 광채
        TextMeshPro text = CreateLevelUpText(root);    // Level Up! 텍스트

        var so = new SerializedObject(effect);
        so.FindProperty("_riseParticle").objectReferenceValue = rise;
        so.FindProperty("_glowParticle").objectReferenceValue = glow;
        so.FindProperty("_levelUpText").objectReferenceValue = text;
        so.FindProperty("_lifetime").floatValue = 2.5f;
        so.ApplyModifiedProperties();

        string dir = "Assets/Resources/Prefabs/Effects";
        if (!System.IO.Directory.Exists(dir))
            System.IO.Directory.CreateDirectory(dir);

        string path = $"{dir}/LevelUpEffect.prefab";
        bool success;
        PrefabUtility.SaveAsPrefabAsset(root, path, out success);
        Object.DestroyImmediate(root);

        if (success)
        {
            AssetDatabase.Refresh();
            Debug.Log($"[LevelUpEffectCreator] 완료: {path}");
            EditorUtility.DisplayDialog("완료", $"LevelUpEffect 생성!\n{path}", "확인");
        }
    }

    // ── 1. 바닥 주변에서 솟구치는 빛기둥 ──────────────────────────────────
    // 캐릭터 발 주변 원형 배치에서 위로 상승하는 가는 황금빛 기둥들
    static ParticleSystem CreateRiseParticle(GameObject root)
    {
        GameObject go = new GameObject("RiseParticle");
        go.transform.SetParent(root.transform, false);
        go.transform.localPosition = Vector3.zero;

        ParticleSystem ps = go.AddComponent<ParticleSystem>();
        ParticleSystemRenderer r = go.GetComponent<ParticleSystemRenderer>();

        var main = ps.main;
        main.duration = 0.5f;
        main.loop = false;
        main.startLifetime = new ParticleSystem.MinMaxCurve(1.2f, 2.0f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(3.0f, 5.0f);
        main.startSize = new ParticleSystem.MinMaxCurve(0.05f, 0.12f);
        main.startColor = new ParticleSystem.MinMaxGradient(
            new Color(1.0f, 0.95f, 0.3f, 1.0f),   // 밝은 노랑
            new Color(1.0f, 0.75f, 0.1f, 1.0f));   // 황금
        main.gravityModifier = -0.5f;   // 위로 상승
        main.simulationSpace = ParticleSystemSimulationSpace.World;
        main.maxParticles = 60;

        var emission = ps.emission;
        emission.rateOverTime = 0f;
        emission.SetBursts(new[] {
            new ParticleSystem.Burst(0.0f, 30),
            new ParticleSystem.Burst(0.2f, 20),
        });

        // 캐릭터 주변 원형 배치
        var shape = ps.shape;
        shape.enabled = true;
        shape.shapeType = ParticleSystemShapeType.Circle;
        shape.radius = 0.6f;
        shape.radiusThickness = 0.3f;
        shape.arc = 360f;

        // 위로만 이동
        var vel = ps.velocityOverLifetime;
        vel.enabled = true;
        vel.x = new ParticleSystem.MinMaxCurve(0f);
        vel.y = new ParticleSystem.MinMaxCurve(2.5f);
        vel.z = new ParticleSystem.MinMaxCurve(0f);

        // 나타났다가 점점 사라짐
        var sizeOL = ps.sizeOverLifetime;
        sizeOL.enabled = true;
        AnimationCurve sc = new AnimationCurve(
            new Keyframe(0f, 0f),
            new Keyframe(0.08f, 1f),
            new Keyframe(0.6f, 0.7f),
            new Keyframe(1f, 0f));
        sizeOL.size = new ParticleSystem.MinMaxCurve(1f, sc);

        // 노랑 → 흰색 → 투명
        var colorOL = ps.colorOverLifetime;
        colorOL.enabled = true;
        Gradient g = new Gradient();
        g.SetKeys(
            new GradientColorKey[] {
                new GradientColorKey(new Color(1f, 1f, 0.4f), 0f),
                new GradientColorKey(new Color(1f, 0.9f, 0.3f), 0.3f),
                new GradientColorKey(new Color(1f, 1f, 0.8f), 0.7f),
                new GradientColorKey(new Color(1f, 1f, 1f), 1f)
            },
            new GradientAlphaKey[] {
                new GradientAlphaKey(0f,   0f),
                new GradientAlphaKey(1f,   0.1f),
                new GradientAlphaKey(0.8f, 0.6f),
                new GradientAlphaKey(0f,   1f)
            });
        colorOL.color = new ParticleSystem.MinMaxGradient(g);

        // Stretch → 빛기둥처럼 길쭉하게
        r.material = ParticleMat();
        r.renderMode = ParticleSystemRenderMode.Stretch;
        r.velocityScale = 0.2f;
        r.lengthScale = 3.5f;

        return ps;
    }

    // ── 2. 캐릭터 주위 은은한 광채 ────────────────────────────────────────
    // 황금빛 입자들이 캐릭터를 감싸듯 느리게 맴돎
    static ParticleSystem CreateGlowParticle(GameObject root)
    {
        GameObject go = new GameObject("GlowParticle");
        go.transform.SetParent(root.transform, false);
        go.transform.localPosition = new Vector3(0f, 0.5f, 0f);

        ParticleSystem ps = go.AddComponent<ParticleSystem>();
        ParticleSystemRenderer r = go.GetComponent<ParticleSystemRenderer>();

        var main = ps.main;
        main.duration = 1.5f;
        main.loop = false;
        main.startLifetime = new ParticleSystem.MinMaxCurve(1.0f, 2.0f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(0f);
        main.startSize = new ParticleSystem.MinMaxCurve(0.08f, 0.2f);
        main.startColor = new ParticleSystem.MinMaxGradient(
            new Color(1f, 0.95f, 0.3f, 0.9f),
            new Color(1f, 0.8f, 0.1f, 0.7f));
        main.gravityModifier = -0.1f;
        main.simulationSpace = ParticleSystemSimulationSpace.World;
        main.maxParticles = 40;

        var emission = ps.emission;
        emission.rateOverTime = 0f;
        emission.SetBursts(new[] {
            new ParticleSystem.Burst(0f, 25),
        });

        var shape = ps.shape;
        shape.enabled = true;
        shape.shapeType = ParticleSystemShapeType.Circle;
        shape.radius = 0.5f;
        shape.radiusThickness = 1.0f;
        shape.arc = 360f;

        // 천천히 위로 + 궤도 회전
        var vel = ps.velocityOverLifetime;
        vel.enabled = true;
        vel.x = new ParticleSystem.MinMaxCurve(0f);
        vel.y = new ParticleSystem.MinMaxCurve(0.5f);
        vel.z = new ParticleSystem.MinMaxCurve(0f);
        vel.orbitalX = new ParticleSystem.MinMaxCurve(0f);
        vel.orbitalY = new ParticleSystem.MinMaxCurve(2.0f);
        vel.orbitalZ = new ParticleSystem.MinMaxCurve(0f);
        vel.radial = new ParticleSystem.MinMaxCurve(0f);

        // 맥박처럼 크기 변화
        var sizeOL = ps.sizeOverLifetime;
        sizeOL.enabled = true;
        AnimationCurve sc = new AnimationCurve(
            new Keyframe(0f, 0f),
            new Keyframe(0.15f, 1f),
            new Keyframe(0.5f, 0.6f),
            new Keyframe(0.75f, 1f),
            new Keyframe(1f, 0f));
        sizeOL.size = new ParticleSystem.MinMaxCurve(1f, sc);

        // 노랑 → 투명
        var colorOL = ps.colorOverLifetime;
        colorOL.enabled = true;
        Gradient g = new Gradient();
        g.SetKeys(
            new GradientColorKey[] {
                new GradientColorKey(new Color(1f, 1f, 0.3f), 0f),
                new GradientColorKey(new Color(1f, 0.9f, 0.2f), 0.5f),
                new GradientColorKey(new Color(1f, 1f, 0.6f), 1f)
            },
            new GradientAlphaKey[] {
                new GradientAlphaKey(0f,   0f),
                new GradientAlphaKey(1f,   0.15f),
                new GradientAlphaKey(0.8f, 0.7f),
                new GradientAlphaKey(0f,   1f)
            });
        colorOL.color = new ParticleSystem.MinMaxGradient(g);

        r.material = ParticleMat();
        r.renderMode = ParticleSystemRenderMode.Billboard;

        return ps;
    }

    // ── 3. "Level Up!" 텍스트 ─────────────────────────────────────────────
    // 머리 위 2m에 노란 텍스트, 코드에서 위로 올라가며 페이드아웃
    static TextMeshPro CreateLevelUpText(GameObject root)
    {
        GameObject go = new GameObject("LevelUpText");
        go.transform.SetParent(root.transform, false);
        go.transform.localPosition = new Vector3(0f, 2.2f, 0f);

        TextMeshPro tmp = go.AddComponent<TextMeshPro>();
        tmp.text = "Level Up!";
        tmp.fontSize = 5f;
        tmp.fontStyle = FontStyles.Bold;
        tmp.color = new Color(1f, 0.92f, 0.016f, 0f); // 노란색, 시작은 투명
        tmp.alignment = TextAlignmentOptions.Center;

        // 항상 카메라를 바라보도록 (코드에서 처리)
        go.SetActive(false); // 처음엔 비활성화, 코드에서 켬

        return tmp;
    }

    // ── 공통 머티리얼 ─────────────────────────────────────────────────────
    static Material ParticleMat()
    {
        Material mat = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Particle.mat");
        if (mat != null) return mat;

        mat = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Line.mat");
        if (mat != null) return mat;

        return new Material(Shader.Find("Sprites/Default"));
    }
}
#endif