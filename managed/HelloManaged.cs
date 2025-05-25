using System;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    public static unsafe class Entrance
    {
        [UnmanagedCallersOnly]
        public static void Print()
        {
            Console.WriteLine("Hello from CoreCLR!");
        }
    }

}
