using UnityEngine;

public class CameraController : MonoBehaviour
{
    public GameObject MyPlayer { get; set; }

    [SerializeField] private float distance    = 4f;
    [SerializeField] private float sensitivity = 2.0f;
    [SerializeField] private float smoothSpeed = 10.0f;

    public float X { get; set; }
    public float Y { get; set; }

    private const float Y_MIN_LIMIT = -20f;
    private const float Y_MAX_LIMIT =  50f;
    private Quaternion _rotation;

    private void LateUpdate()
    {
        if (MyPlayer == null) return;

        // 마우스 입력 처리
        X += Input.GetAxis("Mouse X") * sensitivity;
        Y -= Input.GetAxis("Mouse Y") * sensitivity;
        Y = ClampAngle(Y, Y_MIN_LIMIT, Y_MAX_LIMIT);

        _rotation = Quaternion.Euler(Y, X, 0);

        Vector3 playerPos = MyPlayer.transform.position;
        Vector3 targetPos = playerPos + _rotation * new Vector3(0, 1.5f, -distance);

        transform.position = Vector3.Lerp(transform.position, targetPos, Time.deltaTime * smoothSpeed);
        transform.LookAt(playerPos + (Vector3.up * 1.0f));
    }

    public void SetPosition()
    {
        if (MyPlayer == null) return;

        _rotation = Quaternion.Euler(Y, X, 0);
        Vector3 targetPos = MyPlayer.transform.position + _rotation * new Vector3(0, 1.5f, -distance);

        transform.position = targetPos;
        transform.LookAt(MyPlayer.transform.position + (Vector3.up * 1.0f));
    }

    private float ClampAngle(float angle, float min, float max)
    {
        if (angle < -360) angle += 360;
        if (angle > 360)  angle -= 360;
        return Mathf.Clamp(angle, min, max);
    }
}
