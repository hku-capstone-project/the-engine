using System;

namespace Game
{
    [AttributeUsage(AttributeTargets.Method)]
    public class StartupSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class UpdateSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class QueryAttribute : Attribute
    {
        public Type[] Components { get; }
        public QueryAttribute(params Type[] comps) => Components = comps;
    }
}