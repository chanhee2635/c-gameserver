// Assets/Editor/ReviveEffectCreator.cs
// Tools > Create ReviveEffect Prefab

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;

public static class ReviveEffectCreator
{
    [MenuItem("Tools/Create ReviveEffect Prefab")]
    public static void Create()
    {
        GameObject root = new GameObject("ReviveEffect");
        root.AddComponent<Poolable>();
        ReviveEffect effect = root.AddComponent<ReviveEffect>();

        ParticleSystem orbit = CreateOrbitRiseParticle(root);  // 주위 돌며 상승
        ParticleSystem ground = CreateGroundParticle(root);      // 바닥 퍼짐

        var so = new SerializedObject(effect);
        so.FindProperty("_orbitRiseParticle").objectReferenceValue = orbit;
        so.FindProperty("_groundParticle").objectReferenceValue = ground;
        so.FindProperty("_lifetime").floatValue = 3.0f;
        so.ApplyModifiedProperties();

        string dir = "Assets/Resources/Prefabs/Effects";
        if (!System.IO.Directory.Exists(dir))
            System.IO.Directory.CreateDirectory(dir);

        string path = $"{dir}/ReviveEffect.prefab";
        bool success;
        PrefabUtility.SaveAsPrefabAsset(root, path, out success);
        Object.DestroyImmediate(root);

        if (success)
        {
            AssetDatabase.Refresh();
            Debug.Log($"[ReviveEffectCreator] 완료: {path}");
            EditorUtility.DisplayDialog("완료", $"ReviveEffect 생성!\n{path}", "확인");
        }
    }

    // ── 1. 주위를 돌며 올라가는 초록빛 ──────────────────────────────────────
    // 캐릭터 주위 원형 배치 → orbitalY 회전 + 위로 상승
    // 나선형으로 올라가는 시각 효과
    static ParticleSystem CreateOrbitRiseParticle(GameObject root)
    {
        GameObject go = new GameObject("OrbitRiseParticle");
        go.transform.SetParent(root.transform, false);
        go.transform.localPosition = new Vector3(0f, 0.1f, 0f);

        ParticleSystem ps = go.AddComponent<ParticleSystem>();
        ParticleSystemRenderer r = go.GetComponent<ParticleSystemRenderer>();

        var main = ps.main;
        main.duration = 1.5f;
        main.loop = false;
        main.startLifetime = new ParticleSystem.MinMaxCurve(1.5f, 2.5f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(0f);
        main.startSize = new ParticleSystem.MinMaxCurve(0.06f, 0.18f);
        main.startColor = new ParticleSystem.MinMaxGradient(
            new Color(0.2f, 1.0f, 0.4f, 1.0f),   // 밝은 초록
            new Color(0.5f, 1.0f, 0.6f, 0.9f));   // 연한 초록
        main.gravityModifier = 0f;
        main.simulationSpace = ParticleSystemSimulationSpace.World;
        main.maxParticles = 80;

        var emission = ps.emission;
        emission.rateOverTime = 0f;
        emission.SetBursts(new[] {
            new ParticleSystem.Burst(0.0f, 30),
            new ParticleSystem.Burst(0.4f, 25),
            new ParticleSystem.Burst(0.8f, 20),
        });

        // 캐릭터 주위 원형 배치 (반지름 0.5)
        var shape = ps.shape;
        shape.enabled = true;
        shape.shapeType = ParticleSystemShapeType.Circle;
        shape.radius = 0.5f;
        shape.radiusThickness = 0.2f;
        shape.arc = 360f;

        // 위로 상승 + 궤도 회전 (나선 효과)
        var vel = ps.velocityOverLifetime;
        vel.enabled = true;
        vel.x = new ParticleSystem.MinMaxCurve(0f);
        vel.y = new ParticleSystem.MinMaxCurve(1.8f);   // 위로 상승 속도
        vel.z = new ParticleSystem.MinMaxCurve(0f);
        vel.orbitalX = new ParticleSystem.MinMaxCurve(0f);
        vel.orbitalY = new ParticleSystem.MinMaxCurve(4.0f);   // 회전 속도 (빠를수록 나선 촘촘)
        vel.orbitalZ = new ParticleSystem.MinMaxCurve(0f);
        vel.radial = new ParticleSystem.MinMaxCurve(-0.2f);  // 약간 안쪽으로

        // 나타났다 사라짐
        var sizeOL = ps.sizeOverLifetime;
        sizeOL.enabled = true;
        AnimationCurve sc = new AnimationCurve(
            new Keyframe(0f, 0f),
            new Keyframe(0.12f, 1f),
            new Keyframe(0.6f, 0.8f),
            new Keyframe(1f, 0f));
        sizeOL.size = new ParticleSystem.MinMaxCurve(1f, sc);

        // 연두 → 밝은 초록 → 흰색 → 투명
        var colorOL = ps.colorOverLifetime;
        colorOL.enabled = true;
        Gradient g = new Gradient();
        g.SetKeys(
            new GradientColorKey[] {
                new GradientColorKey(new Color(0.3f, 1.0f, 0.3f), 0f),
                new GradientColorKey(new Color(0.2f, 1.0f, 0.5f), 0.3f),
                new GradientColorKey(new Color(0.6f, 1.0f, 0.7f), 0.7f),
                new GradientColorKey(new Color(1.0f, 1.0f, 1.0f), 1f)
            },
            new GradientAlphaKey[] {
                new GradientAlphaKey(0f,   0f),
                new GradientAlphaKey(1f,   0.1f),
                new GradientAlphaKey(0.9f, 0.6f),
                new GradientAlphaKey(0f,   1f)
            });
        colorOL.color = new ParticleSystem.MinMaxGradient(g);

        r.material = ParticleMat();
        r.renderMode = ParticleSystemRenderMode.Billboard;
        r.sortingOrder = 1;

        return ps;
    }

    // ── 2. 바닥 퍼짐 ────────────────────────────────────────────────────────
    // 부활 순간 바닥에서 초록빛 링이 퍼져나감
    static ParticleSystem CreateGroundParticle(GameObject root)
    {
        GameObject go = new GameObject("GroundParticle");
        go.transform.SetParent(root.transform, false);
        go.transform.localPosition = new Vector3(0f, 0.05f, 0f);
        go.transform.localRotation = Quaternion.Euler(90f, 0f, 0f);

        ParticleSystem ps = go.AddComponent<ParticleSystem>();
        ParticleSystemRenderer r = go.GetComponent<ParticleSystemRenderer>();

        var main = ps.main;
        main.duration = 0.3f;
        main.loop = false;
        main.startLifetime = new ParticleSystem.MinMaxCurve(0.4f, 0.7f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(3f, 5f);
        main.startSize = new ParticleSystem.MinMaxCurve(0.06f, 0.14f);
        main.startColor = new ParticleSystem.MinMaxGradient(
            new Color(0.2f, 1.0f, 0.4f, 1.0f),
            new Color(0.7f, 1.0f, 0.7f, 0.9f));
        main.gravityModifier = 0f;
        main.simulationSpace = ParticleSystemSimulationSpace.World;
        main.maxParticles = 80;

        var emission = ps.emission;
        emission.rateOverTime = 0f;
        emission.SetBursts(new[] {
            new ParticleSystem.Burst(0.0f, 60),
            new ParticleSystem.Burst(0.2f, 40),
        });

        var shape = ps.shape;
        shape.enabled = true;
        shape.shapeType = ParticleSystemShapeType.Circle;
        shape.radius = 0.05f;
        shape.radiusThickness = 0f;
        shape.arc = 360f;

        var sizeOL = ps.sizeOverLifetime;
        sizeOL.enabled = true;
        AnimationCurve sc = new AnimationCurve(
            new Keyframe(0f, 1f),
            new Keyframe(0.5f, 0.5f),
            new Keyframe(1f, 0f));
        sizeOL.size = new ParticleSystem.MinMaxCurve(1f, sc);

        var colorOL = ps.colorOverLifetime;
        colorOL.enabled = true;
        Gradient g = new Gradient();
        g.SetKeys(
            new GradientColorKey[] {
                new GradientColorKey(new Color(0.3f, 1.0f, 0.3f), 0f),
                new GradientColorKey(new Color(0.7f, 1.0f, 0.7f), 1f)
            },
            new GradientAlphaKey[] {
                new GradientAlphaKey(1f, 0f),
                new GradientAlphaKey(0f, 1f)
            });
        colorOL.color = new ParticleSystem.MinMaxGradient(g);

        r.material = ParticleMat();
        r.renderMode = ParticleSystemRenderMode.Billboard;

        return ps;
    }

    // ── 공통 머티리얼 ────────────────────────────────────────────────────────
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