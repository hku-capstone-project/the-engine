using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Linq.Expressions;

namespace Game
{
    public static unsafe class PluginBootstrap
    {
        // host exports:
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr HostGetProcDel([MarshalAs(UnmanagedType.LPStr)] string name);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegStartupDel(IntPtr fn);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegUpdateDel(IntPtr fn);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegPerEntDel(IntPtr fn);

        // your engine callbacks:
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate uint CreateEntityDel();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddTransformDel(uint e, Transform t);

        // generic marker—the real delegates are built dynamically:
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ManagedPerEntityDel();

        // keep alive
        static List<Delegate> _pinned = new();

        [UnmanagedCallersOnly]
        public static void RegisterAll(
          IntPtr hostGetProcPtr
        )
        {
            var hostGet = Marshal.GetDelegateForFunctionPointer<HostGetProcDel>(hostGetProcPtr);

            var hostStartup = Marshal.GetDelegateForFunctionPointer<HostRegStartupDel>(hostGet("HostRegisterStartup"));
            var hostUpdate = Marshal.GetDelegateForFunctionPointer<HostRegUpdateDel>(hostGet("HostRegisterUpdate"));
            var hostPerEnt = Marshal.GetDelegateForFunctionPointer<HostRegPerEntDel>(hostGet("HostRegisterPerEntityUpdate"));

            // bind your engine callbacks into GameScript:
            GameScript.CreateEntity =
              Marshal.GetDelegateForFunctionPointer<GameScript.CreateEntityDel>(
                hostGet("CreateEntity"));
            GameScript.AddTransform =
              Marshal.GetDelegateForFunctionPointer<GameScript.AddTransformDel>(
                hostGet("AddTransform"));

            // scan for systems
            foreach (var m in Assembly.GetExecutingAssembly()
                                     .GetTypes()
                                     .SelectMany(t => t.GetMethods(
                                         BindingFlags.Static |
                                         BindingFlags.Public |
                                         BindingFlags.NonPublic)))
            {
                // --- STARTUP ---
                if (m.GetCustomAttribute<StartupSystemAttribute>() != null
                    && m.ReturnType == typeof(void)
                    && m.GetParameters().Length == 0)
                {
                    var del = (ManagedPerEntityDel)Delegate.CreateDelegate(
                                typeof(ManagedPerEntityDel), m);
                    _pinned.Add(del);
                    hostStartup(Marshal.GetFunctionPointerForDelegate(del));
                }

                // --- PER-ENTITY UPDATE ---
                if (m.GetCustomAttribute<UpdateSystemAttribute>() != null)
                {
                    var query = m.GetCustomAttribute<QueryAttribute>()
                             ?? throw new InvalidOperationException($"[UpdateSystem] {m.Name} needs [Query]");
                    var comps = query.Components;
                    var pars = m.GetParameters();

                    // expect signature: (float dt, C1* p1, C2* p2, ..., Cn* pn)
                    if (pars.Length != 1 + comps.Length || pars[0].ParameterType != typeof(float))
                        throw new InvalidOperationException($"{m.Name} signature mismatch");

                    // build delegate type: (float, C1*, C2*,..., void)
                    var types = pars.Select(p => p.ParameterType)
                                     .Concat(new[] { typeof(void) })
                                     .ToArray();
                    var delType = Expression.GetDelegateType(types);
                    var del = Delegate.CreateDelegate(delType, m);
                    _pinned.Add(del);

                    var fnPtr = Marshal.GetFunctionPointerForDelegate(del);
                    hostPerEnt(fnPtr);
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

        [UpdateSystem, Query(typeof(Transform))]
        public static void UpdateTransform(float dt, Transform* t)
        {
            ref var tr = ref *t;
            tr.position.Y += MathF.Sin(dt) * 0.1f;
        }
    }
}