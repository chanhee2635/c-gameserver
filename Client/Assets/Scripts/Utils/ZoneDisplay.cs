using UnityEngine;

public class ZoneDisplay : MonoBehaviour
{
    [Header("server.json 과 동일하게 (world)")]
    public int mapSize = 1000;   
    public int zoneSize = 20;   
    public int sceneCount = 8;    

    [Header("표시")]
    public float y = 0.05f;                         
    public Color zoneLine = new Color(1f, 1f, 1f, 0.20f);    
    public Color sceneBorderLine = new Color(1f, 0.25f, 0.25f, 0.9f); 
    public bool drawSceneBorders = true;

    Material _mat;

    void OnEnable()
    {
        var sh = Shader.Find("Hidden/Internal-Colored");
        _mat = new Material(sh) { hideFlags = HideFlags.HideAndDontSave };
        _mat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        _mat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        _mat.SetInt("_Cull", (int)UnityEngine.Rendering.CullMode.Off);
        _mat.SetInt("_ZWrite", 0);
    }
    void OnDisable() { if (_mat != null) DestroyImmediate(_mat); }

    int Cols => (zoneSize > 0) ? mapSize / zoneSize : 0;

    int SceneOf(int x, int z)
    {
        if (sceneCount <= 1) return 0;
        int c = Cols;

        int sceneCols = 1;
        for (int i = 1; i * i <= sceneCount; i++)
            if (sceneCount % i == 0) sceneCols = i;
        int sceneRows = sceneCount / sceneCols;

        int sceneX = x * sceneCols / c;
        int sceneZ = z * sceneRows / c;
        return sceneZ * sceneCols + sceneX;
    }

    void OnRenderObject()
    {
        int c = Cols;
        if (_mat == null || c <= 0) return;

        _mat.SetPass(0);
        GL.PushMatrix();
        GL.MultMatrix(transform.localToWorldMatrix);  
        GL.Begin(GL.LINES);

        float end = c * zoneSize;

        GL.Color(zoneLine);
        for (int i = 0; i <= c; i++)
        {
            float p = i * zoneSize;
            GL.Vertex3(p, y, 0); GL.Vertex3(p, y, end);   
            GL.Vertex3(0, y, p); GL.Vertex3(end, y, p);  
        }

        if (drawSceneBorders && sceneCount > 1)
        {
            GL.Color(sceneBorderLine);
            for (int gx = 1; gx < c; gx++)
                for (int gz = 0; gz < c; gz++)
                    if (SceneOf(gx - 1, gz) != SceneOf(gx, gz))
                    { GL.Vertex3(gx * zoneSize, y, gz * zoneSize); GL.Vertex3(gx * zoneSize, y, (gz + 1) * zoneSize); }
            for (int gz = 1; gz < c; gz++)
                for (int gx = 0; gx < c; gx++)
                    if (SceneOf(gx, gz - 1) != SceneOf(gx, gz))
                    { GL.Vertex3(gx * zoneSize, y, gz * zoneSize); GL.Vertex3((gx + 1) * zoneSize, y, gz * zoneSize); }
        }

        GL.End();
        GL.PopMatrix();
    }
}
