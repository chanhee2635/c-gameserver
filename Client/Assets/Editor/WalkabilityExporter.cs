using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.AI;

public class WalkabilityExporter
{
    const int   MAGIC      = 0x57414C4B; 
    const float ORIGIN_X   = 0f;
    const float ORIGIN_Z   = 0f;
    const float CELL_SIZE  = 1.0f;       
    const int   COLS       = 1000;      
    const int   ROWS       = 1000;
    const float SAMPLE_MAX_DIST = 1.5f;  
    const float RAY_HEIGHT = 1000f;     

    [MenuItem("Tools/Export Walkability Grid")]
    public static void Export()
    {
        byte[] cells = new byte[COLS * ROWS];
        int walkable = 0;

        for (int z = 0; z < ROWS; ++z)
        {
            for (int x = 0; x < COLS; ++x)
            {
                float wx = ORIGIN_X + (x + 0.5f) * CELL_SIZE;
                float wz = ORIGIN_Z + (z + 0.5f) * CELL_SIZE;

                Vector3 ground = new Vector3(wx, 0f, wz);
                if (Physics.Raycast(new Vector3(wx, RAY_HEIGHT, wz), Vector3.down,
                                    out RaycastHit rayHit, RAY_HEIGHT * 2f))
                    ground = rayHit.point;

                bool ok = NavMesh.SamplePosition(ground, out _, SAMPLE_MAX_DIST, NavMesh.AllAreas);

                cells[z * COLS + x] = ok ? (byte)0 : (byte)1;
                if (ok) walkable++;
            }
        }

        string path = EditorUtility.SaveFilePanel("Save Walkability Grid", "", "walkmap.bin", "bin");
        if (string.IsNullOrEmpty(path)) return;

        using (var fs = new FileStream(path, FileMode.Create))
        using (var bw = new BinaryWriter(fs))
        {
            bw.Write(MAGIC);
            bw.Write(COLS);
            bw.Write(ROWS);
            bw.Write(CELL_SIZE);
            bw.Write(ORIGIN_X);
            bw.Write(ORIGIN_Z);
            bw.Write(cells);
        }

        Debug.Log($"[Walkability] exported {COLS}x{ROWS} cells, walkable {walkable}/{COLS * ROWS} -> {path}");
    }
}
