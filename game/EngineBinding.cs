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
        public delegate void AddCameraDel(uint e, iCamera c);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddVelocityDel(uint e, Velocity v);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddPlayerDel(uint e, Player p);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddMeshDel(uint e, Mesh m);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddMaterialDel(uint e, Material mat);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void RemoveComponentDel(uint entityId);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DestroyEntityDel(uint entityId);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool IsKeyPressedDel(int keyCode);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void RegisterMeshDel(int meshId, [MarshalAs(UnmanagedType.LPStr)] string meshPath);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void GetMousePositionDel(out float x, out float y);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void GetMouseDeltaDel(out float dx, out float dy);

        public static CreateEntityDel CreateEntity = null!;
        public static AddTransformDel AddTransform = null!;
        public static AddCameraDel AddCamera = null!;
        public static AddVelocityDel AddVelocity = null!;
        public static AddPlayerDel AddPlayer = null!;
        public static AddMeshDel AddMesh = null!;
        public static AddMaterialDel AddMaterial = null!;
        public static RemoveComponentDel RemoveTransform = null!;
        public static RemoveComponentDel RemoveVelocity = null!;
        public static RemoveComponentDel RemovePlayer = null!;
        public static RemoveComponentDel RemoveMesh = null!;
        public static RemoveComponentDel RemoveMaterial = null!;
        public static DestroyEntityDel DestroyEntity = null!;
        public static IsKeyPressedDel IsKeyPressed = null!;
        public static IsKeyPressedDel IsKeyJustPressed = null!;
        public static IsKeyPressedDel IsKeyJustReleased = null!;
        public static RegisterMeshDel RegisterMesh = null!;
        public static GetMousePositionDel GetMousePosition = null!;
        public static GetMouseDeltaDel GetMouseDelta = null!;

        public static void Init(Func<string, IntPtr> getProc)
        {
            CreateEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityDel>(
                                getProc("CreateEntity"));
            AddTransform = Marshal.GetDelegateForFunctionPointer<AddTransformDel>(
                                getProc("AddTransform"));
            AddCamera = Marshal.GetDelegateForFunctionPointer<AddCameraDel>(
                                getProc("AddCamera"));
            AddVelocity = Marshal.GetDelegateForFunctionPointer<AddVelocityDel>(
                                getProc("AddVelocity"));
            AddPlayer = Marshal.GetDelegateForFunctionPointer<AddPlayerDel>(
                                getProc("AddPlayer"));
            AddMesh = Marshal.GetDelegateForFunctionPointer<AddMeshDel>(
                                getProc("AddMesh"));
            AddMaterial = Marshal.GetDelegateForFunctionPointer<AddMaterialDel>(
                                getProc("AddMaterial"));
            RemoveTransform = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentTransform"));
            RemoveVelocity = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentVelocity"));
            RemovePlayer = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentPlayer"));
            RemoveMesh = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentMesh"));
            RemoveMaterial = Marshal.GetDelegateForFunctionPointer<RemoveComponentDel>(
                                getProc("HostRemoveComponentMaterial"));
            DestroyEntity = Marshal.GetDelegateForFunctionPointer<DestroyEntityDel>(
                                getProc("HostDestroyEntity"));
            IsKeyPressed = Marshal.GetDelegateForFunctionPointer<IsKeyPressedDel>(
                                getProc("IsKeyPressed"));
            IsKeyJustPressed = Marshal.GetDelegateForFunctionPointer<IsKeyPressedDel>(
                                getProc("IsKeyJustPressed"));
            IsKeyJustReleased = Marshal.GetDelegateForFunctionPointer<IsKeyPressedDel>(
                                getProc("IsKeyJustReleased"));
            RegisterMesh = Marshal.GetDelegateForFunctionPointer<RegisterMeshDel>(
                                getProc("RegisterMesh"));
            GetMousePosition = Marshal.GetDelegateForFunctionPointer<GetMousePositionDel>(
                                getProc("GetMousePosition"));
            GetMouseDelta = Marshal.GetDelegateForFunctionPointer<GetMouseDeltaDel>(
                                getProc("GetMouseDelta"));
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