using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;

namespace Game
{
    public static unsafe class PluginBootstrap
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegStartupDel(IntPtr fnPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegUpdateDel(IntPtr fnPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr HostGetProcDel(
          [MarshalAs(UnmanagedType.LPStr)] string name);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegBatchDel(IntPtr managedFnPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ManagedBatchUpdateDel(
            float dt,
            Transform* transform
        );

        // ----- these are YOUR managed‐to‐unmanaged SYSTEM callbacks -----
        // non-generic so GetFunctionPointerForDelegate will accept them:
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ManagedStartupFn();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ManagedUpdateFn(float dt);

        // to keep them alive (so the GC won’t collect them)
        static List<Delegate> _pinned = new();

        [UnmanagedCallersOnly]
        public static void RegisterAll(
          IntPtr hostGetProcPtr
        )
        {
            var hostGet = Marshal.GetDelegateForFunctionPointer<HostGetProcDel>(hostGetProcPtr);

            var hostStartup = Marshal.GetDelegateForFunctionPointer<HostRegStartupDel>(hostGet("HostRegisterStartup"));
            var hostUpdate = Marshal.GetDelegateForFunctionPointer<HostRegUpdateDel>(hostGet("HostRegisterUpdate"));

            // bind your engine callbacks into GameScript:
            GameScript.CreateEntity =
              Marshal.GetDelegateForFunctionPointer<GameScript.CreateEntityDel>(
                hostGet("CreateEntity"));
            GameScript.AddTransform =
              Marshal.GetDelegateForFunctionPointer<GameScript.AddTransformDel>(
                hostGet("AddTransform"));

            var hostRegBatch = Marshal.GetDelegateForFunctionPointer<HostRegBatchDel>(
            hostGet("HostRegisterBatchUpdate"));

            // scan for your methods...
            var asm = Assembly.GetExecutingAssembly();
            foreach (var m in asm.GetTypes().SelectMany(t =>
                  t.GetMethods(BindingFlags.Static |
                               BindingFlags.Public |
                               BindingFlags.NonPublic)))
            {
                if (m.GetCustomAttribute<StartupSystemAttribute>() != null
                    && m.ReturnType == typeof(void)
                    && m.GetParameters().Length == 0)
                {
                    // create a non‐generic delegate
                    var del = (ManagedStartupFn)Delegate.CreateDelegate(
                                typeof(ManagedStartupFn), m);
                    _pinned.Add(del);
                    var fnp = Marshal.GetFunctionPointerForDelegate(del);
                    hostStartup(fnp);
                }

                if (m.GetCustomAttribute<UpdateSystemAttribute>() != null)
                {
                    var pars = m.GetParameters();
                    if (m.ReturnType == typeof(void)
                        && pars.Length == 2
                        && pars[0].ParameterType == typeof(float)
                        && pars[1].ParameterType == typeof(Transform*))
                    {
                        // pin the managed batch update
                        var del = (ManagedBatchUpdateDel)Delegate.CreateDelegate(
                                      typeof(ManagedBatchUpdateDel), m);
                        _pinned.Add(del);

                        // register it
                        hostRegBatch(
                          Marshal.GetFunctionPointerForDelegate(del)
                        );
                    }
                }
            }
        }
    }

    public static unsafe class GameScript
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate uint CreateEntityDel();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddTransformDel(uint e, Transform t);

        public static CreateEntityDel CreateEntity;
        public static AddTransformDel AddTransform;

        [StartupSystem]
        public static void InitGameObjects()
        {
            // spawn a 16×16 grid of entities
            const int N = 16;
            for (int x = 0; x < N; x++)
            {
                for (int y = 0; y < N; y++)
                {
                    uint e = CreateEntity();

                    Transform t = new Transform
                    {
                        position = new System.Numerics.Vector3(x * 1.0f, y * 1.0f, 0)
                    };
                    AddTransform(e, t);
                }
            }
        }

        [UpdateSystem]
        public static void UpdateTransform(float dt, Transform* transform)
        {
            ref Transform t = ref *transform;
            t.position.Y += MathF.Sin(dt) * 0.1f;
        }
    }
}