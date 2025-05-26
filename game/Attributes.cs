using System;

namespace Game
{
    [AttributeUsage(AttributeTargets.Method)]
    public class StartupSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class UpdateSystemAttribute : Attribute { }
}