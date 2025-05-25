using System;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform
    {
        public float x, y, z;
    }
}