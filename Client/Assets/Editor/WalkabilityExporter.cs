using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.AI;

// 베이크된 NavMesh를 1m 격자로 샘플링하여 이동가능(0)/장애물(1) 맵을 굽는다.
// 서버 GridMap 과 좌표계/포맷이 일치해야 함:
//   - 월드 [0, COLS*CELL_SIZE) 를 CELL_SIZE 격자로 분할 (origin 0,0)
//   - 셀(cx,cz) 중심 = (origin + (cx+0.5)*CELL, origin + (cz+0.5)*CELL)
//   - 파일: [magic int32][cols int32][rows int32][cellSize float][originX float][originZ float][cols*rows bytes]
public class WalkabilityExporter
{
    const int   MAGIC      = 0x57414C4B; // 'WALK'
    const float ORIGIN_X   = 0f;
    const float ORIGIN_Z   = 0f;
    const float CELL_SIZE  = 1.0f;       // 충돌 격자 셀 크기 (서버 GridMap 과 동일)
    const int   COLS       = 1000;       // mapSize / CELL_SIZE
    const int   ROWS       = 1000;
    const float SAMPLE_MAX_DIST = 1.5f;  // 셀 중심에서 navmesh 까지 허용 거리
    const float RAY_HEIGHT = 1000f;      // 지형 높이 탐색용 레이 시작 높이

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

                // 1) 지형 표면 높이 탐색 (콜라이더 위에서 아래로 레이)
                Vector3 ground = new Vector3(wx, 0f, wz);
                if (Physics.Raycast(new Vector3(wx, RAY_HEIGHT, wz), Vector3.down,
                                    out RaycastHit rayHit, RAY_HEIGHT * 2f))
                    ground = rayHit.point;

                // 2) navmesh 샘플 → 이동 가능 여부 판정
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
