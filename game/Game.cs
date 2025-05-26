using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection.Emit;

namespace Game
{

    // -------------------------------------------------------------------
    // Plugin Bootstrap
    // -------------------------------------------------------------------
    public static unsafe class PluginBootstrap
    {
        // host exports
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr HostGetProcDel(
          [MarshalAs(UnmanagedType.LPStr)] string name
        );

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegStartupDel(IntPtr fn);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void HostRegPerEntDel(
          IntPtr fnPtr,
          int count,
          [MarshalAs(UnmanagedType.LPArray, ArraySubType=UnmanagedType.LPStr)]
      string[]      names
        );

        // your shared engine callbacks are declared *once* in GameScript
        // we will refer to GameScript.CreateEntityDel and AddTransformDel

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public unsafe delegate void NativePerEntityDel(
            float dt,
            void* componentsPtr
        );

        // keep shims & pinned delegates alive
        static List<DynamicMethod> _shims = new();
        static List<NativePerEntityDel> _pinnedShims = new();

        [UnmanagedCallersOnly]
        public static void RegisterAll(IntPtr hostGetPtr)
        {
            var hostGet = Marshal.GetDelegateForFunctionPointer<HostGetProcDel>(hostGetPtr);
            var hostStartup = Marshal.GetDelegateForFunctionPointer<HostRegStartupDel>(
                                hostGet("HostRegisterStartup"));
            var hostPerEnt = Marshal.GetDelegateForFunctionPointer<HostRegPerEntDel>(
                                hostGet("HostRegisterPerEntityUpdate"));

            // bind CreateEntity/AddTransform into GameScript:
            GameScript.CreateEntity = Marshal.GetDelegateForFunctionPointer<GameScript.CreateEntityDel>(
                                         hostGet("CreateEntity"));
            GameScript.AddTransform = Marshal.GetDelegateForFunctionPointer<GameScript.AddTransformDel>(
                                         hostGet("AddTransform"));

            // scan all static methods for systems
            foreach (var m in Assembly.GetExecutingAssembly()
                                      .GetTypes()
                                      .SelectMany(t => t.GetMethods(
                                         BindingFlags.Static |
                                         BindingFlags.Public |
                                         BindingFlags.NonPublic)))
            {
                // ---- STARTUP ----
                if (m.GetCustomAttribute<StartupSystemAttribute>() != null
                 && m.ReturnType == typeof(void)
                 && m.GetParameters().Length == 0)
                {
                    // make a tiny stub: void stub() { real(); }
                    var stub = new DynamicMethod(
                      "stub_" + m.Name,
                      typeof(void),
                      Type.EmptyTypes,
                      typeof(PluginBootstrap),
                      skipVisibility: true
                    );
                    var il = stub.GetILGenerator();
                    il.Emit(OpCodes.Call, m);
                    il.Emit(OpCodes.Ret);
                    _shims.Add(stub);

                    // bind to host
                    var ptr = stub.CreateDelegate(typeof(GameScript.ManagedStartupDel));
                    var fnp = Marshal.GetFunctionPointerForDelegate((GameScript.ManagedStartupDel)ptr);
                    hostStartup(fnp);
                }

                // ---- PER‐ENTITY UPDATE ----
                if (m.GetCustomAttribute<UpdateSystemAttribute>() != null)
                {
                    var query = m.GetCustomAttribute<QueryAttribute>()
                             ?? throw new InvalidOperationException($"{m.Name} missing [Query]");
                    var comps = query.Components;

                    // for brevity demo only 1‐component here
                    if (comps.Length == 1)
                    {
                        var compType = comps[0];
                        var compName = compType.Name;

                        // emit: void shim(float dt, void* comps)
                        var shim = new DynamicMethod(
                          "shim_" + m.Name,
                          typeof(void),
                          new[] { typeof(float), typeof(void*) },
                          typeof(PluginBootstrap),
                          skipVisibility: true
                        );
                        var il = shim.GetILGenerator();

                        // push dt
                        il.Emit(OpCodes.Ldarg_0);

                        // load components[0]
                        il.Emit(OpCodes.Ldarg_1);    // void*
                        il.Emit(OpCodes.Ldc_I4_0);   // index 0
                        il.Emit(OpCodes.Conv_I);     // native int
                        il.Emit(OpCodes.Add);
                        il.Emit(OpCodes.Ldind_I);    // read ptr
                        il.Emit(OpCodes.Conv_U);     // native ptr

                        // call real system: (float, T*)
                        il.EmitCall(OpCodes.Call, m, null);
                        il.Emit(OpCodes.Ret);

                        _shims.Add(shim);

                        // wrap the DynamicMethod in our Cdecl delegate
                        var nativeDel = (NativePerEntityDel)
                            shim.CreateDelegate(typeof(NativePerEntityDel));
                        _pinnedShims.Add(nativeDel);

                        // now get a real Cdecl function-pointer
                        var fnPtr = Marshal.GetFunctionPointerForDelegate(nativeDel);

                        // register it (count=1, just the one component name)
                        hostPerEnt(fnPtr, 1, new[] { compName });

                        continue;
                    }

                    throw new NotSupportedException(
                      $"Only 1‐component queries supported; saw {comps.Length}"
                    );
                }
            }
        }
    }
    public static unsafe class GameScript
    {
        // engine → managed callbacks
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate uint CreateEntityDel();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void AddTransformDel(uint e, Transform t);

        // startup signature
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ManagedStartupDel();

        public static CreateEntityDel CreateEntity;
        public static AddTransformDel AddTransform;

        [StartupSystem]
        public static void InitGameObjects()
        {
            const int N = 16;
            for (int x = 0; x < N; x++)
                for (int y = 0; y < N; y++)
                {
                    uint e = CreateEntity();
                    AddTransform(e, new Transform
                    {
                        position = new System.Numerics.Vector3(x, y, 0)
                    });
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