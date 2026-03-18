using System.Collections;
using UnityEngine;
using TMPro;

/// <summary>
/// 레벨업 이펙트
/// - 바닥 주변에서 빛줄기 상승
/// - 머리 위 "Level Up!" 텍스트가 노란색으로 떠오름
/// </summary>
public class LevelUpEffect : MonoBehaviour
{
    [Header("파티클")]
    [SerializeField] ParticleSystem _riseParticle;   // 바닥에서 올라오는 빛기둥
    [SerializeField] ParticleSystem _glowParticle;   // 캐릭터 주위 은은한 광채

    [Header("텍스트")]
    [SerializeField] TextMeshPro _levelUpText;       // "Level Up!" 텍스트

    [Header("타이밍")]
    [SerializeField] float _lifetime = 2.5f;

    Poolable _poolable;
    Coroutine _returnCoroutine;

    void Awake()
    {
        _poolable = GetComponent<Poolable>();
    }

    void OnEnable()
    {
        Play(_riseParticle);
        Play(_glowParticle);

        // ★ 런타임에 아웃라인 설정 (에디터 모드 머티리얼 누수 방지)
        if (_levelUpText != null)
        {
            _levelUpText.outlineWidth = 0.2f;
            _levelUpText.outlineColor = new Color32(120, 60, 0, 255);
        }

        if (_returnCoroutine != null)
            StopCoroutine(_returnCoroutine);
        _returnCoroutine = StartCoroutine(RunEffect());
    }

    void Play(ParticleSystem ps)
    {
        if (ps == null) return;
        ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
        ps.Play();
    }

    IEnumerator RunEffect()
    {
        // 텍스트 애니메이션
        if (_levelUpText != null)
            StartCoroutine(AnimateLevelUpText());

        yield return new WaitForSeconds(_lifetime);

        if (_poolable != null)
            Managers.Pool.Push(_poolable);
        else
            Destroy(gameObject);
    }

    IEnumerator AnimateLevelUpText()
    {
        if (_levelUpText == null) yield break;

        _levelUpText.gameObject.SetActive(true);
        _levelUpText.text = "Level Up!";
        _levelUpText.color = new Color(1f, 0.92f, 0.016f, 0f); // 노란색, 투명

        Vector3 startLocalPos = _levelUpText.transform.localPosition;
        Vector3 endLocalPos = startLocalPos + Vector3.up * 1.2f;

        float duration = 1.8f;
        float fadeInEnd = 0.2f;   // 0~0.2: 페이드인
        float fadeOutStart = 0.7f; // 0.7~1.0: 페이드아웃

        float t = 0f;
        while (t < duration)
        {
            t += Time.deltaTime;
            float ratio = t / duration;

            // 위로 부드럽게 이동
            _levelUpText.transform.localPosition = Vector3.Lerp(startLocalPos, endLocalPos,
                Mathf.SmoothStep(0f, 1f, ratio));

            // 알파: 페이드인 → 유지 → 페이드아웃
            float alpha;
            if (ratio < fadeInEnd)
                alpha = Mathf.Lerp(0f, 1f, ratio / fadeInEnd);
            else if (ratio < fadeOutStart)
                alpha = 1f;
            else
                alpha = Mathf.Lerp(1f, 0f, (ratio - fadeOutStart) / (1f - fadeOutStart));

            // 노란색 유지 + 알파 적용
            _levelUpText.color = new Color(1f, 0.92f, 0.016f, alpha);

            // 텍스트가 항상 카메라를 바라보도록
            if (Camera.main != null)
                _levelUpText.transform.rotation = Camera.main.transform.rotation;

            yield return null;
        }

        _levelUpText.gameObject.SetActive(false);
        _levelUpText.transform.localPosition = startLocalPos;
    }
}