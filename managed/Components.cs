using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform
    {
        public Vector3 position;
    }

    // FIXME:
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Model
    {
        // Marshals as a C‐style (null-terminated) ANSI string.
        // For UTF-16 use UnmanagedType.LPWStr + CharSet=CharSet.Unicode
        [MarshalAs(UnmanagedType.LPStr)]
        public string modelPath;
    }
}
