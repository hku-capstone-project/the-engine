using System;
using System.Runtime.InteropServices;

namespace HelloManaged
{
    public static unsafe class GameScript
    {
        // Delegates matching the C ABI
        public delegate uint CreateEntityDel();
        public delegate void AddTransformDel(uint e, Transform t);

        static CreateEntityDel CreateEntity;
        static AddTransformDel AddTransform;


        [UnmanagedCallersOnly]
        public static void Init(IntPtr createEntityPtr, IntPtr addTransformPtr)
        {
            CreateEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityDel>(createEntityPtr);
            AddTransform = Marshal.GetDelegateForFunctionPointer<AddTransformDel>(addTransformPtr);

            // spawn a 16×16 grid of entities
            const int N = 16;
            for (int x = 0; x < N; x++)
            {
                for (int y = 0; y < N; y++)
                {
                    uint e = CreateEntity();
                    AddTransform(e, new Transform
                    {
                        x = x * 1.0f,
                        y = y * 1.0f,
                        z = 0
                    });
                }
            }
        }

        [UnmanagedCallersOnly]
        public static void Update(float dt, IntPtr transformPtr, int count)
        {
            // wrap as Span<Transform>
            var span = new Span<Transform>(transformPtr.ToPointer(), count);

            // simple bobbing in Z
            for (int i = 0; i < span.Length; i++)
            {
                ref var t = ref span[i];
                t.z = (float)(Math.Sin((t.x + t.y) + dt * 3.0) * 0.5);
            }
        }
    }
}