using System;

namespace Game
{
    [AttributeUsage(AttributeTargets.Method)]
    public class StartupSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method)]
    public class UpdateSystemAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public class QueryAttribute : Attribute
    {
        /// <summary>
        /// The sequence of component‐types this system reads or writes.
        /// Order here must match the order of pointer parameters in your method.
        /// </summary>
        public Type[] Components { get; }

        public QueryAttribute(params Type[] components)
        {
            Components = components;
        }
    }
}