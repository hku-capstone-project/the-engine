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

    public static unsafe class Entrance
    {
        [UnmanagedCallersOnly]
        public static void MutateStruct(IntPtr pMyStruct)
        {
            // cast to pointer
            MyStruct* s = (MyStruct*)pMyStruct.ToPointer();

            // mutate fields
            s->x += 100;
            s->y += 200;

            Console.WriteLine($"[C#] I added (100,200): now (x,y)=({s->x},{s->y})");
        }
    }
}
