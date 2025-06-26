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
    // GLFW Key codes (from GLFW/glfw3.h)
    public static class Keys
    {
        public const int GLFW_KEY_SPACE = 32;
        public const int GLFW_KEY_W = 87;
        public const int GLFW_KEY_A = 65;
        public const int GLFW_KEY_S = 83;
        public const int GLFW_KEY_D = 68;
        public const int GLFW_KEY_ESCAPE = 256;
    }

    public static class GameSystems
    {
        private static float _jumpTimer = 0;
        private static float _testTimer = 0;  // New: Test timer
        private static bool _testPhase1Complete = false;  // New: Phase 1 completion flag
        private static bool _testPhase2Complete = false;  // New: Phase 2 completion flag
        private static uint _testEntityWithMesh = 0;
        private static uint _testEntityForDeletion = 0;
        private static StreamWriter _logWriter = null;
        
        [StartupSystem]
        public static void CreateTestEntities()
        {
            // Initialize logging system
            InitializeLogging();

            // add a monkey here
            uint monkeyId = EngineBindings.CreateEntity();
            var transform = new Transform { position = new Vector3(0, 1, 0) };
            EngineBindings.AddTransform(monkeyId, transform);
            var velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(monkeyId, velocity);
            
            // 添加Player组件，让猴子可以被PlayerSystem处理
            var player = new Player { isJumping = false, jumpForce = 8.0f };
            EngineBindings.AddPlayer(monkeyId, player);
            
            Log($"Created monkey entity with ID {monkeyId} - Transform, Velocity, and Player components added");
        }

        private static void InitializeLogging()
        {
            try
            {
                // Get absolute path of project root directory
                string projectRoot = Environment.GetEnvironmentVariable("ENGINE_ROOT") ??
                                   Path.GetFullPath(Path.Combine(Directory.GetCurrentDirectory(), "..", ".."));

                // Create logs folder in project root
                string logsDir = Path.Combine(projectRoot, "logs");
                Directory.CreateDirectory(logsDir);

                // Create timestamped log file
                string timestamp = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
                string logPath = Path.Combine(logsDir, $"game_log_{timestamp}.txt");
                _logWriter = new StreamWriter(logPath, true);
                _logWriter.AutoFlush = true;

                Console.WriteLine($"=== Logging System Initialized Successfully ===");
                Console.WriteLine($"Log file path: {logPath}");
                _logWriter.WriteLine($"=== Logging System Initialized Successfully ===");
                _logWriter.WriteLine($"Log file path: {logPath}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Logging system initialization failed: {ex.Message}");
            }
        }

        private static void Log(string message)
        {
            try
            {
                string timestamp = DateTime.Now.ToString("HH:mm:ss.fff");
                string logMessage = $"[{timestamp}] {message}";

                // Output to both console and log file
                Console.WriteLine(logMessage);
                _logWriter?.WriteLine(logMessage);
                _logWriter?.Flush();  // Ensure immediate file write
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Log write failed: {ex.Message}");
            }
        }

        // 物理系统 - 处理重力和基本物理
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity))]
        public static void PhysicsSystem(float dt, ref Transform transform, ref Velocity velocity)
        {
            // 应用重力
            velocity.velocity.Y -= 9.81f * dt;
            
            // 更新位置
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;

            // 地面碰撞检测
            if (transform.position.Y < 0)
            {
                transform.position.Y = 0;
                velocity.velocity.Y = 0;
            }

            Log($"Physics - Y: {transform.position.Y:F2}, VelY: {velocity.velocity.Y:F2}");
        }

        // 玩家控制系统 - 处理输入和跳跃
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        public static void PlayerSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        {
            // 使用新的游戏输入系统 - 简洁可靠的按键检测
            bool spaceJustPressed = EngineBindings.IsKeyJustPressed(Keys.GLFW_KEY_SPACE);
            bool isOnGround = transform.position.Y <= 0.1f;
            
            // 移动输入检测
            bool leftPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_A) ;
            bool rightPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_D) ;
            bool upPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_W) ;
            bool downPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_S) ;
            
            // 水平移动速度
            const float moveSpeed = 5.0f;
            float horizontalInput = 0.0f;
            float verticalInput = 0.0f;
            
            if (leftPressed) horizontalInput -= 1.0f;
            if (rightPressed) horizontalInput += 1.0f;
            if (upPressed) verticalInput -= 1.0f;   
            if (downPressed) verticalInput += 1.0f;  
            
            // 应用水平移动（不影响重力）
            velocity.velocity.X = horizontalInput * moveSpeed;
            velocity.velocity.Z = verticalInput * moveSpeed;
            
            // 跳跃逻辑：只有在按下瞬间且在地面时才跳跃
            if (spaceJustPressed && isOnGround)
            {
                velocity.velocity.Y = 8.0f; // 跳跃力度
                Log("Player jumped!");
            }
            
            // 调试：每秒记录一次状态
            if (_testTimer > 1.0f)
            {
                bool currentPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_SPACE);
                Log($"Input - Space: {currentPressed}, JustPressed: {spaceJustPressed}, Move: ({horizontalInput:F1}, {verticalInput:F1})");
                _testTimer = 0;
            }
            _testTimer += dt;
        }

        // // Basic physics system
        // [UpdateSystem]
        // [Query(typeof(Transform), typeof(Velocity))]
        // public static void PhysicsSystem(float dt, ref Transform transform, ref Velocity velocity)
        // {
        //     transform.position.X += velocity.velocity.X * dt;
        //     transform.position.Y += velocity.velocity.Y * dt;
        //     transform.position.Z += velocity.velocity.Z * dt;

        //     velocity.velocity.Y -= 9.8f * dt;  // Gravity

        //     if (transform.position.Y < 0)
        //     {
        //         transform.position.Y = 0;
        //         velocity.velocity.Y = 0;
        //     }

        //     Log($"PhysicsSystem - Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}");
        // }

        // // Player system
        // [UpdateSystem]
        // [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        // public static void PlayerSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        // {
        //     _jumpTimer += dt;
        //     if (_jumpTimer >= 0.1f)
        //     {
        //         player.isJumping = true;
        //         _jumpTimer = 0;
        //     }

        //     if (player.isJumping)
        //     {
        //         velocity.velocity.Y = player.jumpForce;
        //         player.isJumping = false;
        //     }

        //     Log($"PlayerSystem - Player Entity - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}, Jumping: {player.isJumping}");
        // }

        // // Test multi-component query: Mesh + Material
        // [UpdateSystem]
        // [Query(typeof(Transform), typeof(Mesh), typeof(Material))]
        // public static void RenderSystem(float dt, ref Transform transform, ref Mesh mesh, ref Material material)
        // {
        //     Log($"🎨 RenderSystem - Entity - Position: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2}), " +
        //                      $"ModelID: {mesh.modelId}, Color: ({material.color.X:F2}, {material.color.Y:F2}, {material.color.Z:F2})");

        //     // Simple animation
        //     transform.position.X += 0.1f * dt;
        //     material.color.X = 0.5f + 0.5f * MathF.Sin(_testTimer * 0.01f);
        // }

        // // Deletion test system - using time control
        // [UpdateSystem]
        // [Query(typeof(Transform))]
        // public static void DeletionTestSystem(float dt, ref Transform transform)
        // {
        //     // Only execute deletion logic for test entity (identified by X and Z coordinates, Y will change due to gravity)
        //     if (transform.position.X == -5 && transform.position.Z == -5)
        //     {
        //         _testTimer += dt;

        //         // Phase 1: Test component deletion after 0.1s
        //         if (!_testPhase1Complete && _testTimer >= 0.1f)
        //         {
        //             Log("🔥 === Test Point 3: Component Deletion Feature ===");
        //             Log($"Removing Velocity component from entity ID {_testEntityForDeletion}...");
        //             EngineBindings.RemoveVelocity(_testEntityForDeletion);
        //             Log("✅ Velocity component removed. This entity should no longer appear in PhysicsSystem.");

        //             Log($"Removing Material component from entity ID {_testEntityWithMesh}...");
        //             EngineBindings.RemoveMaterial(_testEntityWithMesh);
        //             Log("✅ Material component removed. This entity should no longer appear in RenderSystem.");

        //             _testPhase1Complete = true;
        //         }

        //         // Phase 2: Test entity deletion after 0.2s
        //         if (!_testPhase2Complete && _testTimer >= 0.2f)
        //         {
        //             Log("💀 === Test Point 3: Entity Deletion Feature ===");
        //             Log($"Deleting entity ID {_testEntityForDeletion}...");
        //             EngineBindings.DestroyEntity(_testEntityForDeletion);
        //             Log("✅ Entity deleted. This entity should no longer appear in any system.");

        //             _testPhase2Complete = true;
        //         }
        //     }
        // }
    }
}

