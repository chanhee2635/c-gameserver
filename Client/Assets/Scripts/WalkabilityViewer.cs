using System.IO;
using UnityEngine;

// walkmap.bin(WalkabilityExporter 결과)을 읽어 씬 뷰에 격자를 겹쳐 그린다.
// 사용법: 빈 GameObject에 붙이고, 그 오브젝트를 움직이며 주변 장애물(빨강)을 확인.
// 100만 셀 전부를 기즈모로 그리면 에디터가 멈추므로 drawRadius 반경만 그린다.
// 전체 개요는 ContextMenu의 "Export PNG"로 한 장에 본다.
[ExecuteAlways]
public class WalkabilityViewer : MonoBehaviour
{
    const int MAGIC = 0x57414C4B; // 'WALK' (서버/익스포터와 동일)

    [Tooltip("비우면 ../../GameServer/walkmap.bin 사용")]
    public string binPath = "";
    [Tooltip("이 오브젝트 주변 몇 m만 그릴지 (전체를 그리면 멈춤)")]
    public float  drawRadius   = 30f;
    public bool   drawWalkable = false;
    public Color  blockedColor  = new Color(1f, 0f, 0f, 0.45f);
    public Color  walkableColor = new Color(0f, 1f, 0f, 0.12f);

    int    _cols, _rows;
    float  _cellSize, _originX, _originZ;
    byte[] _cells;
    string _loadedPath;

    string ResolvePath() =>
        string.IsNullOrEmpty(binPath)
            ? Path.GetFullPath(Path.Combine(Application.dataPath, "../../Common/Map/walkmap.bin"))
            : binPath;

    [ContextMenu("Reload")]
    void Reload() { _cells = null; LoadIfNeeded(); }

    void LoadIfNeeded()
    {
        string path = ResolvePath();
        if (_cells != null && _loadedPath == path) return;

        _loadedPath = path;
        _cells = null;
        if (!File.Exists(path)) return;

        using var br = new BinaryReader(File.OpenRead(path));
        if (br.ReadInt32() != MAGIC) { Debug.LogWarning($"[Walkability] bad magic: {path}"); return; }
        _cols     = br.ReadInt32();
        _rows     = br.ReadInt32();
        _cellSize = br.ReadSingle();
        _originX  = br.ReadSingle();
        _originZ  = br.ReadSingle();
        _cells    = br.ReadBytes(_cols * _rows);
        Debug.Log($"[Walkability] loaded {_cols}x{_rows} cell={_cellSize} origin=({_originX},{_originZ})");
    }

    void OnDrawGizmos()
    {
        LoadIfNeeded();
        if (_cells == null) return;

        Vector3 center = transform.position;
        int half = Mathf.CeilToInt(drawRadius / _cellSize);
        int ccx  = Mathf.FloorToInt((center.x - _originX) / _cellSize);
        int ccz  = Mathf.FloorToInt((center.z - _originZ) / _cellSize);
        Vector3 size = new Vector3(_cellSize, 0.05f, _cellSize) * 0.95f;

        for (int z = ccz - half; z <= ccz + half; ++z)
        {
            if (z < 0 || z >= _rows) continue;
            for (int x = ccx - half; x <= ccx + half; ++x)
            {
                if (x < 0 || x >= _cols) continue;

                byte v = _cells[z * _cols + x];
                if (v == 0 && !drawWalkable) continue;

                Gizmos.color = (v == 1) ? blockedColor : walkableColor;
                Vector3 c = new Vector3(
                    _originX + (x + 0.5f) * _cellSize,
                    center.y,
                    _originZ + (z + 0.5f) * _cellSize);
                Gizmos.DrawCube(c, size);
            }
        }
    }

    // 전체 격자를 PNG 한 장으로: 장애물=검정, 이동가능=흰색
    [ContextMenu("Export PNG")]
    void ExportPng()
    {
        _cells = null;
        LoadIfNeeded();
        if (_cells == null) { Debug.LogWarning("[Walkability] walkmap.bin 없음"); return; }

        Texture2D tex = new Texture2D(_cols, _rows, TextureFormat.RGB24, false);
        Color32 white = new Color32(255, 255, 255, 255);
        Color32 black = new Color32(0, 0, 0, 255);
        var px = new Color32[_cols * _rows];
        for (int i = 0; i < px.Length; ++i)
            px[i] = (_cells[i] == 1) ? black : white;   // 1=장애물
        tex.SetPixels32(px);
        tex.Apply();

        string outPath = Path.ChangeExtension(ResolvePath(), ".png");
        File.WriteAllBytes(outPath, tex.EncodeToPNG());
        DestroyImmediate(tex);
        Debug.Log($"[Walkability] PNG 저장: {outPath}");
    }
}
