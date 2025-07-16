using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Game
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform
    {
        public Vector3 position;
        public Vector3 rotation; // 欧拉角
        public Vector3 scale; // 缩放
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct iCamera
    {
        public float fov; // 视场角
        public float nearPlane; // 近裁剪面
        public float farPlane; // 远裁剪面
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
        public float metallic ;
        public float roughness ;
        public float occlusion ;
        public Vector3 emissive ;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MeshDefinition
    {
        public int modelId;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string modelPath;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GameStats
    {
        public int killCount;   // 击杀数量
        public float gameTime;  // 游戏时间（秒）
    }
    
}
