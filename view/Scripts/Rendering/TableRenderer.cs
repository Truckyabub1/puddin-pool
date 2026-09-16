using System;
using System.Collections.Generic;
using PuddinPool.Native;

namespace PuddinPool.Rendering
{
    public struct MeshData
    {
        public List<float[]> vertices; // [x, y, z]
        public List<int> triangles;
        public List<float[]> normals;
        public List<float[]> uvs;

        public static MeshData Create()
        {
            return new MeshData
            {
                vertices = new List<float[]>(),
                triangles = new List<int>(),
                normals = new List<float[]>(),
                uvs = new List<float[]>()
            };
        }
    }

    public class TableRenderer
    {
        public float TableWidth { get; private set; } = 2.24f;
        public float TableHeight { get; private set; } = 1.12f;
        public float BallRadius { get; private set; } = 0.028575f;

        public MeshData SlateMesh { get; private set; }
        public MeshData CushionMesh { get; private set; }
        public List<float[]> PocketPositions { get; private set; }

        public BallTransform[] CurrentTransforms { get; private set; }
        public int ActiveBallCount { get; private set; }

        public TableRenderer(float width = 2.24f, float height = 1.12f)
        {
            TableWidth = width;
            TableHeight = height;
            CurrentTransforms = new BallTransform[16];
            PocketPositions = new List<float[]>();

            BuildProceduralTable();
        }

        public void BuildProceduralTable()
        {
            // 1. Procedural Slate Mesh
            SlateMesh = MeshData.Create();
            // 4 Quad Vertices
            SlateMesh.vertices.Add(new float[] { 0.0f, 0.0f, 0.0f });
            SlateMesh.vertices.Add(new float[] { TableWidth, 0.0f, 0.0f });
            SlateMesh.vertices.Add(new float[] { TableWidth, TableHeight, 0.0f });
            SlateMesh.vertices.Add(new float[] { 0.0f, TableHeight, 0.0f });

            SlateMesh.uvs.Add(new float[] { 0.0f, 0.0f });
            SlateMesh.uvs.Add(new float[] { 1.0f, 0.0f });
            SlateMesh.uvs.Add(new float[] { 1.0f, 1.0f });
            SlateMesh.uvs.Add(new float[] { 0.0f, 1.0f });

            for (int i = 0; i < 4; ++i)
                SlateMesh.normals.Add(new float[] { 0.0f, 0.0f, 1.0f });

            SlateMesh.triangles.AddRange(new int[] { 0, 1, 2, 0, 2, 3 });

            // 2. Pocket Cutout Centers (6 standard pockets)
            PocketPositions.Clear();
            float cornerCut = 0.08f;
            PocketPositions.Add(new float[] { cornerCut, cornerCut, 0.0f });                                // Bottom-Left
            PocketPositions.Add(new float[] { TableWidth * 0.5f, 0.04f, 0.0f });                            // Bottom-Center
            PocketPositions.Add(new float[] { TableWidth - cornerCut, cornerCut, 0.0f });                  // Bottom-Right
            PocketPositions.Add(new float[] { cornerCut, TableHeight - cornerCut, 0.0f });                  // Top-Left
            PocketPositions.Add(new float[] { TableWidth * 0.5f, TableHeight - 0.04f, 0.0f });              // Top-Center
            PocketPositions.Add(new float[] { TableWidth - cornerCut, TableHeight - cornerCut, 0.0f });    // Top-Right

            // 3. Cushion Boundaries Mesh
            CushionMesh = MeshData.Create();
            float railHeight = 0.04f;
            float railThickness = 0.05f;

            // Generate cushion rails around slate
            AddRailSegment(CushionMesh, 0.12f, -railThickness, TableWidth - 0.12f, 0.0f, railHeight); // Bottom
            AddRailSegment(CushionMesh, 0.12f, TableHeight, TableWidth - 0.12f, TableHeight + railThickness, railHeight); // Top
            AddRailSegment(CushionMesh, -railThickness, 0.12f, 0.0f, TableHeight - 0.12f, railHeight); // Left
            AddRailSegment(CushionMesh, TableWidth, 0.12f, TableWidth + railThickness, TableHeight - 0.12f, railHeight); // Right
        }

        private void AddRailSegment(MeshData mesh, float x0, float y0, float x1, float y1, float z)
        {
            int baseIdx = mesh.vertices.Count;
            mesh.vertices.Add(new float[] { x0, y0, z });
            mesh.vertices.Add(new float[] { x1, y0, z });
            mesh.vertices.Add(new float[] { x1, y1, z });
            mesh.vertices.Add(new float[] { x0, y1, z });

            for (int i = 0; i < 4; ++i)
            {
                mesh.normals.Add(new float[] { 0.0f, 0.0f, 1.0f });
                mesh.uvs.Add(new float[] { 0.0f, 0.0f });
            }

            mesh.triangles.AddRange(new int[] { baseIdx, baseIdx + 1, baseIdx + 2, baseIdx, baseIdx + 2, baseIdx + 3 });
        }

        public void UpdateBallTransforms(BallTransform[] transforms, int count)
        {
            ActiveBallCount = Math.Min(count, CurrentTransforms.Length);
            for (int i = 0; i < ActiveBallCount; ++i)
            {
                CurrentTransforms[i] = transforms[i];
            }
        }
    }
}
