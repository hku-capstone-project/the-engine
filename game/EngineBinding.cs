using System;
using System.Runtime.InteropServices;

namespace Game
{
    internal static class EngineBindings
    {
        // private delegate types
        private delegate uint CreateEntityNative();
        private delegate void AddTransformNative(uint e, Transform t);
        private delegate void AddVelocityNative(uint e, Velocity v);

        private static CreateEntityNative _createEntity = null!;
        private static AddTransformNative _addTransform = null!;
        private static AddVelocityNative _addVelocity = null!;
        private static bool _isInitialized;

        /// <summary>
        /// Must be called once by the host (PluginBootstrap.RegisterAll).
        /// </summary>
        public static void Init(Func<string, IntPtr> getProc)
        {
            if (_isInitialized) return;
            _isInitialized = true;

            _createEntity = Marshal.GetDelegateForFunctionPointer<CreateEntityNative>(
                                getProc("CreateEntity"));
            _addTransform = Marshal.GetDelegateForFunctionPointer<AddTransformNative>(
                                getProc("AddTransform"));
            _addVelocity = Marshal.GetDelegateForFunctionPointer<AddVelocityNative>(
                                getProc("AddVelocity"));
        }

        public static uint CreateEntity()
        {
            if (!_isInitialized)
                throw new InvalidOperationException("EngineBindings.Init() must be called first.");
            return _createEntity();
        }

        public static void AddTransform(uint e, Transform t)
        {
            if (!_isInitialized)
                throw new InvalidOperationException("EngineBindings.Init() must be called first.");
            _addTransform(e, t);
        }

        public static void AddVelocity(uint e, Velocity v)
        {
            if (!_isInitialized)
                throw new InvalidOperationException("EngineBindings.Init() must be called first.");
            _addVelocity(e, v);
        }
    }
}