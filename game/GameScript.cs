using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection.Emit;

namespace Game
{
    public static unsafe class GameScript
    {
        [StartupSystem]
        public static void InitGameObjects()
        {
            const int N = 4;
            for (int x = 0; x < N; x++)
                for (int y = 0; y < N; y++)
                {
                    uint e = EngineBindings.CreateEntity();
                    EngineBindings.AddTransform(e, new Transform
                    {
                        position = new System.Numerics.Vector3(x, y, 0)
                    });
                    EngineBindings.AddVelocity(e, new Velocity
                    {
                        velocity = new System.Numerics.Vector3(
                            (x - N / 2) * 0.1f,
                            (y - N / 2) * 0.1f,
                            0
                        )
                    });
                }
        }

        [UpdateSystem, Query(typeof(Transform))]
        public static void UpdateTransform(float dt, Transform* t)
        {
            ref var tr = ref *t;
            tr.position.Y += MathF.Sin(dt) * 0.1f;
        }

        [UpdateSystem, Query(typeof(Velocity))]
        public static void UpdateVelocity(float dt, Velocity* v)
        {
            ref var vel = ref *v;
            vel.velocity += new System.Numerics.Vector3(0, -9.81f * dt, 0);
        }
    }
}
