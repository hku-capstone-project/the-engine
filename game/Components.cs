using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Game
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform
    {
        public Vector3 position;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Velocity
    {
        public Vector3 velocity;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Player
    {
        public bool isJumping;
        public float jumpForce;
    }

    // // FIXME:
    // [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    // public struct Model
    // {
    //     // Marshals as a C‐style (null-terminated) ANSI string.
    //     // For UTF-16 use UnmanagedType.LPWStr + CharSet=CharSet.Unicode
    //     [MarshalAs(UnmanagedType.LPStr)]
    //     public string modelPath;
    // }

    [StructLayout(LayoutKind.Sequential)]
    public struct Mesh
    {
        public int modelId;  // 使用ID而不是字符串路径，避免marshalling问题
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Material
    {
        public Vector3 color;
        public float metallic = 0.0;
        public float roughness = 0.5;
        public float occlusion = 1.0;
        public Vector3 emissive = new Vector3(0.0f);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MeshDefinition
    {
        public int modelId;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string modelPath;
    }
    
}
