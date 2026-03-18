using System.Collections;
using UnityEngine;

/// <summary>
/// 부활 이펙트
/// - 초록빛 입자들이 캐릭터 주위를 돌며 위로 올라감
/// </summary>
public class ReviveEffect : MonoBehaviour
{
    [Header("파티클")]
    [SerializeField] ParticleSystem _orbitRiseParticle;  // 주위를 돌며 상승하는 빛
    [SerializeField] ParticleSystem _groundParticle;     // 바닥 퍼짐

    [Header("타이밍")]
    [SerializeField] float _lifetime = 3.0f;

    Poolable _poolable;
    Coroutine _returnCoroutine;

    void Awake()
    {
        _poolable = GetComponent<Poolable>();
    }

    void OnEnable()
    {
        Play(_orbitRiseParticle);
        Play(_groundParticle);

        if (_returnCoroutine != null)
            StopCoroutine(_returnCoroutine);
        _returnCoroutine = StartCoroutine(ReturnToPool());
    }

    void Play(ParticleSystem ps)
    {
        if (ps == null) return;
        ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
        ps.Play();
    }

    IEnumerator ReturnToPool()
    {
        yield return new WaitForSeconds(_lifetime);

        if (_poolable != null)
            Managers.Pool.Push(_poolable);
        else
            Destroy(gameObject);
    }
}