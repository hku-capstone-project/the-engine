using System;
using System.Runtime.InteropServices;

namespace Game
{
    internal static class EngineBindings
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate uint CreateEntityDel();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddTransformDel(uint e, Transform t);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddVelocityDel(uint e, Velocity v);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddPlayerDel(uint e, Player p);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void RemoveComponentDel(uint entityId);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DestroyEntityDel(uint entityId);

        public static CreateEntityDel CreateEntity = null!;
        public static AddTransformDel AddTransform = null!;
        public static AddVelocityDel AddVelocity = null!;
        public static AddPlayerDel AddPlayer = null!;
        public static RemoveComponentDel RemoveTransform = null!;
        public static RemoveComponentDel RemoveVelocity = null!;
        public static RemoveComponentDel RemovePlayer = null!;
        public static DestroyEntityDel DestroyEntity = null!;

        public static void Init(Func<string, IntPtr> getProc)
        {
            CreateEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityDel>(
                                getProc("CreateEntity"));
            AddTransform = Marshal.GetDelegateForFunctionPointer<AddTransformDel>(
                                getProc("AddTransform"));
            AddVelocity = Marshal.GetDelegateForFunctionPointer<AddVelocityDel>(
                                getProc("AddVelocity"));
            AddPlayer = Marshal.GetDelegateForFunctionPointer<AddPlayerDel>(
                                getProc("AddPlayer"));
            RemoveTransform = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentTransform"));
            RemoveVelocity = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentVelocity"));
            RemovePlayer = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentPlayer"));
            DestroyEntity = Marshal.GetDelegateForFunctionPointer<DestroyEntityDel>(
                                getProc("HostDestroyEntity"));
        }
    }
}

// using System;
// using System.Runtime.InteropServices;

// namespace Game
// {
//     internal static class EngineBindings
//     {
//         public delegate uint CreateEntityDel();
//         public delegate void AddTransformDel(uint e, Transform t);
//         public delegate void AddVelocityDel(uint e, Velocity v);
//         public delegate void AddPlayerDel(uint e, Player p);

//         public static CreateEntityDel CreateEntity = null!;
//         public static AddTransformDel AddTransform = null!;
//         public static AddVelocityDel AddVelocity = null!;
//         public static AddPlayerDel AddPlayer = null!;

//         public static void Init(Func<string, IntPtr> getProc)
//         {
//             CreateEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityDel>(
//                                 getProc("CreateEntity"));
//             AddTransform = Marshal.GetDelegateForFunctionPointer<AddTransformDel>(
//                                 getProc("AddTransform"));
//             AddVelocity = Marshal.GetDelegateForFunctionPointer<AddVelocityDel>(
//                                 getProc("AddVelocity"));
//             AddPlayer = Marshal.GetDelegateForFunctionPointer<AddPlayerDel>(
//                                 getProc("AddPlayer"));
//         }
//     }
// }