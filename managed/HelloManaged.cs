using System;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    public static unsafe class Entrance
    {
        [UnmanagedCallersOnly]
        public static void MutateStruct(IntPtr pTransform)
        {
            // cast to pointer
            Transform* s = (Transform*)pTransform.ToPointer();

            // mutate fields
            s->x += 100.0f;
            s->y += 200.0f;
            s->z = 0.0f;
        }
    }
}
