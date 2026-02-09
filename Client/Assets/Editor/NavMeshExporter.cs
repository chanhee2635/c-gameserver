using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.AI;

public class NavMeshExporter : MonoBehaviour
{
    [MenuItem("Tools/Export NavMesh to OBJ")]
    public static void Export()
    {
        // 현재 씬에 Bake 된 NavMesh 데이터를 가져옴
        NavMeshTriangulation triangulation = NavMesh.CalculateTriangulation();

        if (triangulation.vertices.Length == 0)
        {
            Debug.Log("추출할 NavMesh 데이터가 없습니다. 먼저 Bake를 진행해주세요!");
            return;
        }

        // 저장 경로 선택
        string path = EditorUtility.SaveFilePanel("Save NavMesh OBJ", "", "SceneNavMesh.obj", "obj");
        if (string.IsNullOrEmpty(path)) return;

        StringBuilder sb = new StringBuilder();

        foreach (Vector3 v in triangulation.vertices)
        {
            sb.AppendLine(string.Format("v {0} {1} {2}", v.x, v.y, v.z));
        }

        sb.AppendLine();

        // 인덱스 정보 쓰기
        for (int i = 0; i < triangulation.indices.Length; i += 3)
        {
            sb.AppendLine(string.Format("f {0} {1} {2}",
                triangulation.indices[i] + 1,
                triangulation.indices[i + 1] + 1,
                triangulation.indices[i + 2] + 1));
        }

        File.WriteAllText(path, sb.ToString());
        Debug.Log($"네비메쉬 추출 완료: {path}");
    }
}
