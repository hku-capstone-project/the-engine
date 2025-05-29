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
        private static int _frameCount = 0;
        private static uint _testEntityWithMesh = 0;
        private static uint _testEntityForDeletion = 0;
        private static bool _deletion600Done = false;
        private static bool _deletion800Done = false;

        [StartupSystem]
        public static void CreateTestEntities()
        {
            Console.WriteLine("🚀 === 开始验证新功能 ===");
            
            // 创建基础实体：玩家和普通物体
            uint playerId = EngineBindings.CreateEntity();
            var transform1 = new Transform { position = new Vector3(0, 0, 0) };
            EngineBindings.AddTransform(playerId, transform1);
            var velocity1 = new Velocity { velocity = new Vector3(1, 0, 0) };
            EngineBindings.AddVelocity(playerId, velocity1);
            var player = new Player { isJumping = false, jumpForce = 5.0f };
            EngineBindings.AddPlayer(playerId, player);

            uint objectId = EngineBindings.CreateEntity();
            var transform2 = new Transform { position = new Vector3(0, 2, 0) };
            EngineBindings.AddTransform(objectId, transform2);
            var velocity2 = new Velocity { velocity = new Vector3(0.5f, 0, 0) };
            EngineBindings.AddVelocity(objectId, velocity2);
            
            // 测试点2: 创建带有Mesh和Material组件的实体
            Console.WriteLine("🎯 测试点2: 创建带有Mesh和Material组件的实体...");
            _testEntityWithMesh = EngineBindings.CreateEntity();
            
            var transform3 = new Transform { position = new Vector3(5, 0, 5) };
            EngineBindings.AddTransform(_testEntityWithMesh, transform3);
            
            var mesh = new Mesh { modelId = 1 };
            EngineBindings.AddMesh(_testEntityWithMesh, mesh);
            
            var material = new Material { color = new Vector3(1.0f, 0.5f, 0.2f) };
            EngineBindings.AddMaterial(_testEntityWithMesh, material);
            
            Console.WriteLine($"✅ 创建实体ID: {_testEntityWithMesh}，具有Transform、Mesh和Material组件");
            
            // 创建测试删除的实体
            _testEntityForDeletion = EngineBindings.CreateEntity();
            var transformForDeletion = new Transform { position = new Vector3(-5, 3, -5) };
            EngineBindings.AddTransform(_testEntityForDeletion, transformForDeletion);
            var velocityForDeletion = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(_testEntityForDeletion, velocityForDeletion);
            
            Console.WriteLine($"✅ 创建测试删除实体ID: {_testEntityForDeletion}，具有Transform和Velocity组件");
            Console.WriteLine("💡 预期：第600帧删除组件，第800帧删除实体");
        }

        // 基础物理系统
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity))]
        public static void PhysicsSystem(float dt, ref Transform transform, ref Velocity velocity)
        {
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;
            
            velocity.velocity.Y -= 9.8f * dt;  // 重力

            if (transform.position.Y < 0)
            {
                transform.position.Y = 0;
                velocity.velocity.Y = 0;
            }
            
            Console.WriteLine($"PhysicsSystem - Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}");
        }

        // 玩家系统
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        public static void PlayerSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        {
            _jumpTimer += dt;
            if (_jumpTimer >= 2.0f)
            {
                player.isJumping = true;
                _jumpTimer = 0;
            }

            if (player.isJumping)
            {
                velocity.velocity.Y = player.jumpForce;
                player.isJumping = false;
            }
            
            Console.WriteLine($"PlayerSystem - Player Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}, Jumping: {player.isJumping}");
        }

        // 验证多组件查询：Mesh + Material
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Mesh), typeof(Material))]
        public static void RenderSystem(float dt, ref Transform transform, ref Mesh mesh, ref Material material)
        {
            Console.WriteLine($"🎨 RenderSystem - Entity - Position: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2}), " +
                             $"ModelID: {mesh.modelId}, Color: ({material.color.X:F2}, {material.color.Y:F2}, {material.color.Z:F2})");
                             
            // 简单动画
            transform.position.X += 0.1f * dt;
            material.color.X = 0.5f + 0.5f * MathF.Sin(_frameCount * 0.01f);
        }

        // 删除测试系统 - 固定帧数触发
        [UpdateSystem]
        [Query(typeof(Transform))]
        public static void DeletionTestSystem(float dt, ref Transform transform)
        {
            _frameCount++;
            
            // 第3000帧：测试组件删除
            if (_frameCount == 3000 && !_deletion600Done)
            {
                Console.WriteLine($"🔥 === 第{_frameCount}帧：测试点3 - 组件删除功能 ===");
                Console.WriteLine($"从实体ID {_testEntityForDeletion} 移除Velocity组件...");
                EngineBindings.RemoveVelocity(_testEntityForDeletion);
                Console.WriteLine("✅ Velocity组件已移除。该实体应该不再出现在PhysicsSystem中。");
                
                Console.WriteLine($"从实体ID {_testEntityWithMesh} 移除Material组件...");
                EngineBindings.RemoveMaterial(_testEntityWithMesh);
                Console.WriteLine("✅ Material组件已移除。该实体应该不再出现在RenderSystem中。");
                
                _deletion600Done = true;
                return; // 只执行一次
            }
            
            // 第3100帧：测试实体删除
            if (_frameCount == 3100 && !_deletion800Done)
            {
                Console.WriteLine($"💀 === 第{_frameCount}帧：测试点3 - 实体删除功能 ===");
                Console.WriteLine($"删除实体ID {_testEntityForDeletion}...");
                EngineBindings.DestroyEntity(_testEntityForDeletion);
                Console.WriteLine("✅ 实体已删除。该实体应该不再出现在任何系统中。");
                
                _deletion800Done = true;
                return; // 只执行一次
            }
            
            // 每100帧输出一次帧数信息
            if (_frameCount % 100 == 0)
            {
                Console.WriteLine($"📊 当前帧数: {_frameCount}");
            }
        }
    }
}
