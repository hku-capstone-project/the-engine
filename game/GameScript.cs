using System;
using System.IO;
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
        private static float _testTimer = 0;  // 新增：测试计时器
        private static bool _testPhase1Complete = false;  // 新增：第一阶段完成标志
        private static bool _testPhase2Complete = false;  // 新增：第二阶段完成标志
        private static uint _testEntityWithMesh = 0;
        private static uint _testEntityForDeletion = 0;
        private static StreamWriter _logWriter = null;

        [StartupSystem]
        public static void CreateTestEntities()
        {
            // 初始化日志系统
            InitializeLogging();
            
            Log("🚀 === 开始验证新功能 ===");
            
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
            Log("🎯 测试点2: 创建带有Mesh和Material组件的实体...");
            _testEntityWithMesh = EngineBindings.CreateEntity();
            
            var transform3 = new Transform { position = new Vector3(5, 0, 5) };
            EngineBindings.AddTransform(_testEntityWithMesh, transform3);
            
            var mesh = new Mesh { modelId = 1 };
            EngineBindings.AddMesh(_testEntityWithMesh, mesh);
            
            var material = new Material { color = new Vector3(1.0f, 0.5f, 0.2f) };
            EngineBindings.AddMaterial(_testEntityWithMesh, material);
            
            Log($"✅ 创建实体ID: {_testEntityWithMesh}，具有Transform、Mesh和Material组件");
            
            // 创建测试删除的实体
            _testEntityForDeletion = EngineBindings.CreateEntity();
            var transformForDeletion = new Transform { position = new Vector3(-5, 3, -5) };
            EngineBindings.AddTransform(_testEntityForDeletion, transformForDeletion);
            var velocityForDeletion = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(_testEntityForDeletion, velocityForDeletion);
            
            Log($"✅ 创建测试删除实体ID: {_testEntityForDeletion}，具有Transform和Velocity组件");
            Log("💡 预期：0.1秒后删除组件，0.2秒后删除实体");  // 修改提示信息
        }

        private static void InitializeLogging()
        {
            try
            {
                // 获取项目根目录的绝对路径
                string projectRoot = Environment.GetEnvironmentVariable("ENGINE_ROOT") ?? 
                                   Path.GetFullPath(Path.Combine(Directory.GetCurrentDirectory(), "..", ".."));
                
                // 在项目根目录下创建logs文件夹
                string logsDir = Path.Combine(projectRoot, "logs");
                Directory.CreateDirectory(logsDir);

                // 创建带时间戳的日志文件
                string timestamp = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
                string logPath = Path.Combine(logsDir, $"game_log_{timestamp}.txt");
                _logWriter = new StreamWriter(logPath, true);
                _logWriter.AutoFlush = true;

                Console.WriteLine($"=== 日志系统初始化成功 ===");
                Console.WriteLine($"日志文件路径: {logPath}");
                _logWriter.WriteLine($"=== 日志系统初始化成功 ===");
                _logWriter.WriteLine($"日志文件路径: {logPath}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"日志系统初始化失败: {ex.Message}");
            }
        }

        private static void Log(string message)
        {
            try
            {
                string timestamp = DateTime.Now.ToString("HH:mm:ss.fff");
                string logMessage = $"[{timestamp}] {message}";
                
                // 同时输出到控制台和日志文件
                Console.WriteLine(logMessage);
                _logWriter?.WriteLine(logMessage);
                _logWriter?.Flush();  // 确保立即写入文件
            }
            catch (Exception ex)
            {
                Console.WriteLine($"日志写入失败: {ex.Message}");
            }
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
            
            Log($"PhysicsSystem - Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}");
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
            
            Log($"PlayerSystem - Player Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}, Jumping: {player.isJumping}");
        }

        // 验证多组件查询：Mesh + Material
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Mesh), typeof(Material))]
        public static void RenderSystem(float dt, ref Transform transform, ref Mesh mesh, ref Material material)
        {
            Log($"🎨 RenderSystem - Entity - Position: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2}), " +
                             $"ModelID: {mesh.modelId}, Color: ({material.color.X:F2}, {material.color.Y:F2}, {material.color.Z:F2})");
                             
            // 简单动画
            transform.position.X += 0.1f * dt;
            material.color.X = 0.5f + 0.5f * MathF.Sin(_testTimer * 0.01f);
        }

        // 删除测试系统 - 使用时间控制
        [UpdateSystem]
        [Query(typeof(Transform))]
        public static void DeletionTestSystem(float dt, ref Transform transform)
        {
            // 只在删除测试实体时执行删除逻辑（通过X和Z坐标识别，Y坐标会因重力变化）
            if (transform.position.X == -5 && transform.position.Z == -5)
            {
                _testTimer += dt;
                
                // 阶段1: 0.1秒后测试组件删除
                if (!_testPhase1Complete && _testTimer >= 0.1f)
                {
                    Log("🔥 === 测试点3: 组件删除功能 ===");
                    Log($"从实体ID {_testEntityForDeletion} 移除Velocity组件...");
                    EngineBindings.RemoveVelocity(_testEntityForDeletion);
                    Log("✅ Velocity组件已移除。该实体应该不再出现在PhysicsSystem中。");
                    
                    Log($"从实体ID {_testEntityWithMesh} 移除Material组件...");
                    EngineBindings.RemoveMaterial(_testEntityWithMesh);
                    Log("✅ Material组件已移除。该实体应该不再出现在RenderSystem中。");
                    
                    _testPhase1Complete = true;
                }
                
                // 阶段2: 0.2秒后测试实体删除
                if (!_testPhase2Complete && _testTimer >= 0.2f)
                {
                    Log("💀 === 测试点3: 实体删除功能 ===");
                    Log($"删除实体ID {_testEntityForDeletion}...");
                    EngineBindings.DestroyEntity(_testEntityForDeletion);
                    Log("✅ 实体已删除。该实体应该不再出现在任何系统中。");
                    
                    _testPhase2Complete = true;
                }
            }
        }
    }
}
