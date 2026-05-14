using System.Collections;
using UnityEngine;

/// <summary>
/// 부활 이펙트
/// - 초록빛 파티클이 캐릭터 주변을 감싸며 위로 올라옴
/// </summary>
public class ReviveEffect : MonoBehaviour
{
    [Header("파티클")]
    [SerializeField] private ParticleSystem _orbitRiseParticle;  // 주위를 감싸며 올라오는 빛
    [SerializeField] private ParticleSystem _groundParticle;     // 바닥 효과

    [Header("타이밍")]
    [SerializeField] private float _lifetime = 3.0f;

    private Poolable  _poolable;
    private Coroutine _returnCoroutine;

    private void Awake()
    {
        _poolable = GetComponent<Poolable>();
    }

    private void OnEnable()
    {
        Play(_orbitRiseParticle);
        Play(_groundParticle);

        if (_returnCoroutine != null)
            StopCoroutine(_returnCoroutine);
        _returnCoroutine = StartCoroutine(ReturnToPool());
    }

    private void Play(ParticleSystem ps)
    {
        if (ps == null) return;
        ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
        ps.Play();
    }

    private IEnumerator ReturnToPool()
    {
        yield return new WaitForSeconds(_lifetime);

        if (_poolable != null)
            Managers.Pool.Push(_poolable);
        else
            Destroy(gameObject);
    }
}
