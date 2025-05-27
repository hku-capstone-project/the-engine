using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection.Emit;

namespace Game
{
  public static unsafe class PluginBootstrap
  {
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ManagedStartupDel();

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

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public unsafe delegate void NativePerEntityDel(
        float dt,
        void* componentsPtr
    );

    // keep shims & pinned delegates alive
    static List<DynamicMethod> _shims = new();
    static List<NativePerEntityDel> _pinnedShims = new();

    public static Func<string, IntPtr> GetProc { get; private set; }

    [UnmanagedCallersOnly]
    public static void RegisterAll(IntPtr hostGetPtr)
    {
      var hostGet = Marshal.GetDelegateForFunctionPointer<HostGetProcDel>(hostGetPtr);

      GetProc = name => hostGet(name);
      EngineBindings.Init(GetProc);

      var hostStartup = Marshal.GetDelegateForFunctionPointer<HostRegStartupDel>(
                          hostGet("HostRegisterStartup"));
      var hostPerEnt = Marshal.GetDelegateForFunctionPointer<HostRegPerEntDel>(
                          hostGet("HostRegisterPerEntityUpdate"));

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
          var ptr = stub.CreateDelegate(typeof(ManagedStartupDel));
          var fnp = Marshal.GetFunctionPointerForDelegate((ManagedStartupDel)ptr);
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

            // load components[0] (which is T*)
            // This sequence correctly pushes the T* (e.g., Transform*) onto the stack.
            // Ldarg_1 loads the 'comps' argument, which is effectively void** (pointer to the component pointer).
            // The sequence Ldarg_1, Ldc_I4_0, Conv_I, Add, Ldind_I, Conv_U results in
            // the actual component pointer (T*) being pushed onto the stack.
            il.Emit(OpCodes.Ldarg_1);    // void* (this is &ptrs[0] from C++)
            il.Emit(OpCodes.Ldc_I4_0);   // offset 0.
            il.Emit(OpCodes.Conv_I);     // Convert offset to native int.
            il.Emit(OpCodes.Add);        // Add offset to base address: &ptrs[0] + 0 = &ptrs[0].
            il.Emit(OpCodes.Ldind_I);    // Dereference: loads the value at &ptrs[0], which is ptrs[0] (the T*).
            il.Emit(OpCodes.Conv_U);     // Convert T* to UIntPtr (unsigned native int).

            // call real system: (float, ref T)
            // The MethodInfo 'm' now refers to a method like UpdateTransform(float, ref Transform).
            // The CLR handles passing the T* (on stack as UIntPtr) to a 'ref T' parameter.
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
            $"Only 1-component queries supported; saw {comps.Length}"
          );
        }
      }
    }
  }
}
