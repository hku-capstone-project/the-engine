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
          var compNames = comps.Select(c => c.Name).ToArray();

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

          // 为每个组件生成加载代码
          for (int i = 0; i < comps.Length; i++)
          {
            // 加载组件指针数组
            il.Emit(OpCodes.Ldarg_1);    // void* (this is &ptrs[0] from C++)
            il.Emit(OpCodes.Ldc_I4, i * IntPtr.Size);  // 当前组件索引 * 指针大小
            il.Emit(OpCodes.Conv_I);     // 转换为native int
            il.Emit(OpCodes.Add);        // 计算当前组件指针的地址
            il.Emit(OpCodes.Ldind_I);    // 加载组件指针
            il.Emit(OpCodes.Conv_U);     // 转换为UIntPtr
          }

          // 调用实际的系统方法
          il.EmitCall(OpCodes.Call, m, null);
          il.Emit(OpCodes.Ret);

          _shims.Add(shim);

          // 包装DynamicMethod为Cdecl委托
          var nativeDel = (NativePerEntityDel)
              shim.CreateDelegate(typeof(NativePerEntityDel));
          _pinnedShims.Add(nativeDel);

          // 获取真实的Cdecl函数指针
          var fnPtr = Marshal.GetFunctionPointerForDelegate(nativeDel);

          // 注册系统，传入所有组件名称
          hostPerEnt(fnPtr, comps.Length, compNames);
        }
      }
    }
  }
}
