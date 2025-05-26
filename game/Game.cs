using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;

namespace Game
{
    [AttributeUsage(AttributeTargets.Method)]
    public class StartupSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class UpdateSystemAttribute : Attribute { }

    public static unsafe class PluginBootstrap
    {
        // engine→managed exports:
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegStartupDel(IntPtr fnPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegUpdateDel(IntPtr fnPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr HostGetProcDel(
          [MarshalAs(UnmanagedType.LPStr)] string name);

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
          IntPtr hostRegStartupPtr,
          IntPtr hostRegUpdatePtr,
          IntPtr hostGetProcPtr
        )
        {
            var hostStartup = Marshal.GetDelegateForFunctionPointer<HostRegStartupDel>(hostRegStartupPtr);
            var hostUpdate = Marshal.GetDelegateForFunctionPointer<HostRegUpdateDel>(hostRegUpdatePtr);
            var hostGet = Marshal.GetDelegateForFunctionPointer<HostGetProcDel>(hostGetProcPtr);

            // bind your engine callbacks into GameScript:
            GameScript.CreateEntity =
              Marshal.GetDelegateForFunctionPointer<GameScript.CreateEntityDel>(
                hostGet("CreateEntity"));
            GameScript.AddTransform =
              Marshal.GetDelegateForFunctionPointer<GameScript.AddTransformDel>(
                hostGet("AddTransform"));
            GameScript.GetTransformBuffer =
              Marshal.GetDelegateForFunctionPointer<GameScript.HostGetTransformBufferDel>(
                hostGet("HostGetTransformBuffer"));
            GameScript.GetTransformCount =
              Marshal.GetDelegateForFunctionPointer<GameScript.HostGetTransformCountDel>(
                hostGet("HostGetTransformCount"));

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

                if (m.GetCustomAttribute<UpdateSystemAttribute>() != null
                    && m.ReturnType == typeof(void)
                    && m.GetParameters().Length == 1
                    && m.GetParameters()[0].ParameterType == typeof(float))
                {
                    var del = (ManagedUpdateFn)Delegate.CreateDelegate(
                                typeof(ManagedUpdateFn), m);
                    _pinned.Add(del);
                    var fnp = Marshal.GetFunctionPointerForDelegate(del);
                    hostUpdate(fnp);
                }
            }
        }
    }

    public static unsafe class GameScript
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate uint CreateEntityDel();
        public static CreateEntityDel CreateEntity;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddTransformDel(uint e, Transform t);
        public static AddTransformDel AddTransform;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr HostGetTransformBufferDel();
        public static HostGetTransformBufferDel GetTransformBuffer;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int HostGetTransformCountDel();
        public static HostGetTransformCountDel GetTransformCount;


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
        public static void UpdateTransform(float dt)
        {
            // IntPtr buf = GetTransformBuffer();
            // int cnt = GetTransformCount();
            // var span = new Span<Transform>(buf.ToPointer(), cnt);

            // for (int i = 0; i < cnt; i++)
            // {
            //     ref var t = ref span[i];
            //     t.position.Z = (float)(Math.Sin((t.position.X + t.position.Y) + dt * 3.0) * 0.5);
            // }
        }
    }
}