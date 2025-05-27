using System;
using System.Runtime.InteropServices;

namespace Game
{
    internal static class EngineBindings
    {
        public delegate uint CreateEntityDel();
        public delegate void AddTransformDel(uint e, Transform t);
        public delegate void AddVelocityDel(uint e, Velocity v);

        public static CreateEntityDel CreateEntity = null!;
        public static AddTransformDel AddTransform = null!;
        public static AddVelocityDel AddVelocity = null!;

        public static void Init(Func<string, IntPtr> getProc)
        {
            CreateEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityDel>(
                                getProc("CreateEntity"));
            AddTransform = Marshal.GetDelegateForFunctionPointer<AddTransformDel>(
                                getProc("AddTransform"));
            AddVelocity = Marshal.GetDelegateForFunctionPointer<AddVelocityDel>(
                                getProc("AddVelocity"));
        }
    }
}