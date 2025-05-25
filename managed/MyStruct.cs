using System;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    [StructLayout(LayoutKind.Sequential)]
    public struct MyStruct
    {
        public int x;
        public int y;
    }
}
