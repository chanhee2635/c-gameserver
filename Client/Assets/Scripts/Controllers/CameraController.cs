using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraController : MonoBehaviour
{
    public GameObject MyPlayer;
    private float distance = 4f;
    private float speed = 1.5f;

    Quaternion rotation;
    public float X { get; set; }
    public float Y { get; set; }
    float yMinLimit = -20f;
    float yMaxLimit = 20f;

    private void Start()
    {
        //Cursor.lockState = CursorLockMode.Locked;
        //Cursor.visible = false;
    }

    void LateUpdate()
    {
        if (MyPlayer == null) return;

        RotationCam();

        transform.position = MyPlayer.transform.position + rotation * new Vector3(0, distance * 0.8f, -distance);
        transform.LookAt(MyPlayer.transform.position + (transform.up * 0.5f));
    }

    void RotationCam()
    {
        X += Input.GetAxis("Mouse X") * speed;
        Y -= Input.GetAxis("Mouse Y") * speed;
        Y = ClampAngle(Y, yMinLimit, yMaxLimit);
        rotation = Quaternion.Euler(Y, X, 0);
    }

    float ClampAngle(float angle, float min, float max)
    {
        if (angle < -360) angle += 360;
        else if (angle > 360) angle -= 360;

        return Mathf.Clamp(angle, min, max);
    }
}
