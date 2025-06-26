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

            // previous code, kept for reference
            // {
            // uint playerId = EngineBindings.CreateEntity();
            // var transform1 = new Transform { position = new Vector3(0, 0, 0) };
            // EngineBindings.AddTransform(playerId, transform1);
            // var velocity1 = new Velocity { velocity = new Vector3(1, 0, 0) };
            // EngineBindings.AddVelocity(playerId, velocity1);
            // var player = new Player { isJumping = false, jumpForce = 5.0f };
            // EngineBindings.AddPlayer(playerId, player);

            // uint objectId = EngineBindings.CreateEntity();
            // var transform2 = new Transform { position = new Vector3(0, 2, 0) };
            // EngineBindings.AddTransform(objectId, transform2);
            // var velocity2 = new Velocity { velocity = new Vector3(0.5f, 0, 0) };
            // EngineBindings.AddVelocity(objectId, velocity2);

            // _testEntityWithMesh = EngineBindings.CreateEntity();

            // var transform3 = new Transform { position = new Vector3(5, 0, 5) };
            // EngineBindings.AddTransform(_testEntityWithMesh, transform3);

            // var mesh = new Mesh { modelId = 1 };
            // EngineBindings.AddMesh(_testEntityWithMesh, mesh);

            // var material = new Material { color = new Vector3(1.0f, 0.5f, 0.2f) };
            // EngineBindings.AddMaterial(_testEntityWithMesh, material);

            // // Create test entity for deletion
            // _testEntityForDeletion = EngineBindings.CreateEntity();
            // var transformForDeletion = new Transform { position = new Vector3(-5, 3, -5) };
            // EngineBindings.AddTransform(_testEntityForDeletion, transformForDeletion);
            // var velocityForDeletion = new Velocity { velocity = new Vector3(0, 0, 0) };
            // EngineBindings.AddVelocity(_testEntityForDeletion, velocityForDeletion);
            // }

            // add a monkey here
            uint monkeyId = EngineBindings.CreateEntity();
            var transform = new Transform { position = new Vector3(0, 1, 0) };
            EngineBindings.AddTransform(monkeyId, transform);
            var velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(monkeyId, velocity);
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

        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity))]
        private static void BouncingMonkeySystem(float dt, ref Transform transform, ref Velocity velocity)
        {
            // g = 9.81 m/s^2
            // apply g into velocity, considering dt
            velocity.velocity.Y -= 9.81f * dt;

            // then apply the velocity to the transform
            transform.position.Y += velocity.velocity.Y * dt;

            // if the monkey is below the ground (y < 0), bounce it
            if (transform.position.Y < 0)
            {
                velocity.velocity.Y = -velocity.velocity.Y;
            }

            Log($"BouncingMonkeySystem - Monkey - Position: {transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1}, Velocity: {velocity.velocity.X:F1}, {velocity.velocity.Y:F1}, {velocity.velocity.Z:F1}");
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
