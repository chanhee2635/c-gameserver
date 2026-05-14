using TMPro;
using UnityEngine;

public class UI_Damage : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI _damageText;

    private const float DURATION   = 1.0f;
    private const float MOVE_SPEED = 0.3f;

    private float    _timer;
    private Poolable _pool;
    private Camera   _mainCam;

    private void Start()
    {
        _mainCam = Camera.main;
    }

    public void SetInfo(int damage)
    {
        _damageText.text  = damage.ToString();
        _damageText.color = damage < 0 ? Color.red : Color.blue;
        _timer = 0;

        transform.localPosition += new Vector3(Random.Range(-0.2f, 0.2f), 0, 0);
        _pool = GetComponent<Poolable>();
    }

    private void Update()
    {
        _timer += Time.deltaTime;

        transform.Translate(Vector3.up * MOVE_SPEED * Time.deltaTime);

        Color color = _damageText.color;
        color.a = Mathf.Lerp(1, 0, _timer / DURATION);
        _damageText.color = color;

        if (_timer > DURATION)
            Managers.Pool.Push(_pool);

        transform.rotation = _mainCam.transform.rotation;
    }
}
