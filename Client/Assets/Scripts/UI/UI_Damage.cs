using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class UI_Damage : MonoBehaviour
{
    [SerializeField]
    TextMeshProUGUI DamageText;
    float _duration = 1.0f;
    float _moveSpeed = 0.3f;
    float _timer = 0;
    Poolable _pool;

    public void SetInfo(int damage)
    {
        DamageText.text = damage.ToString();
        DamageText.color = damage < 0 ? Color.red : Color.blue;
        _timer = 0;

        transform.localPosition += new Vector3(Random.Range(-0.2f, 0.2f), 0, 0);
        _pool = GetComponent<Poolable>();
    }

    void Update()
    {
        _timer += Time.deltaTime;

        transform.Translate(Vector3.up * _moveSpeed * Time.deltaTime);

        Color color = DamageText.color;
        color.a = Mathf.Lerp(1, 0, _timer / _duration);
        DamageText.color = color;

        if (_timer > _duration)
            Managers.Pool.Push(_pool);

        transform.rotation = Camera.main.transform.rotation;
    }
}
