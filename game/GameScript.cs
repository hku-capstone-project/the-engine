using System;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection.Emit;
using System.Numerics;

namespace Game
{
    public static class GameSystems
    {
        private static float _jumpTimer = 0;

        [StartupSystem]
        public static void CreateTestEntities()
        {
            // 创建两个实体：
            // 1. 一个同时具有Transform、Velocity和Player组件的实体
            // 2. 一个只有Transform和Velocity的实体
            
            // 实体1：玩家
            uint playerId = EngineBindings.CreateEntity();
            
            var transform1 = new Transform { 
                position = new Vector3(0, 0, 0)
            };
            EngineBindings.AddTransform(playerId, transform1);
            
            var velocity1 = new Velocity {
                velocity = new Vector3(1, 0, 0)
            };
            EngineBindings.AddVelocity(playerId, velocity1);
            
            var player = new Player {
                isJumping = false,
                jumpForce = 5.0f
            };
            EngineBindings.AddPlayer(playerId, player);

            // 实体2：普通物体
            uint objectId = EngineBindings.CreateEntity();
            
            var transform2 = new Transform { 
                position = new Vector3(0, 2, 0)  // 放在玩家上方
            };
            EngineBindings.AddTransform(objectId, transform2);
            
            var velocity2 = new Velocity {
                velocity = new Vector3(0.5f, 0, 0)  // 移动速度比玩家慢
            };
            EngineBindings.AddVelocity(objectId, velocity2);
        }

        // 这个系统只处理有Transform和Velocity的实体
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity))]
        public static void PhysicsSystem(float dt, ref Transform transform, ref Velocity velocity)
        {
            // 基本的物理更新
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;
            
            // 添加重力效果
            velocity.velocity.Y -= 9.8f * dt;  // 重力加速度

            // 地面碰撞检测
            if (transform.position.Y < 0)
            {
                transform.position.Y = 0;
                velocity.velocity.Y = 0;
            }
            
            Console.WriteLine($"PhysicsSystem - Entity - Position: {transform.position.X}, {transform.position.Y}, {transform.position.Z}");
        }

        // 这个系统只处理有Transform、Velocity和Player的实体
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        public static void PlayerSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        {
            // 每2秒触发一次跳跃
            _jumpTimer += dt;
            if (_jumpTimer >= 2.0f)
            {
                player.isJumping = true;
                _jumpTimer = 0;
            }

            // 玩家特有的逻辑 - 只处理跳跃
            if (player.isJumping)
            {
                velocity.velocity.Y = player.jumpForce;
                player.isJumping = false;
            }
            
            Console.WriteLine($"PlayerSystem - Player Entity - Position: {transform.position.X}, {transform.position.Y}, {transform.position.Z}, Jumping: {player.isJumping}");
        }
    }
}
