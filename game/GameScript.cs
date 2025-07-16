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

#if GameScript
    public static class GameSystems
    {
        private static float _testTimer = 0;  // 用于降低日志频率
        private static StreamWriter _logWriter = null;

        // 吸血鬼幸存者游戏变量
        private static uint _playerId = 0;  // 玩家实体ID
        private static uint _cameraId = 0;  // 摄像机实体ID
        private static uint _gameStatsId = 0;  // 游戏统计实体ID
        private static List<uint> _vampireIds = new List<uint>();  // 吸血鬼实体ID列表
        private static Vector3 _playerPosition = Vector3.Zero;  // 玩家位置（全局共享）
        private static Dictionary<uint, float> _vampireSpeeds = new Dictionary<uint, float>();  // 吸血鬼移动速度
        
        // 游戏状态
        private static float _gameStartTime = 0f;  // 游戏开始时间
        private static int _killCount = 0;  // 全局击杀计数器
        
        // 敌人生成变量
        private static int _enemyCount = 10000;  // 敌人数量
        private static float _enemySpawnRadius = 100.0f;  // 敌人生成半径（10x默认）
        private static System.Diagnostics.Stopwatch _creationStopwatch = new System.Diagnostics.Stopwatch();
        private static List<uint> _entitiesToDestroy = new List<uint>();  // 待销毁的实体列表
        
        // 摄像机控制变量
        private static float _cameraYaw = 0f;     // 水平角度（绕Y轴旋转）
        private static float _cameraPitch = 0f;   // 俯仰角度（初始平视 = 0度）
        private static float _mouseSensitivity = 0.0005f;  // 鼠标灵敏度（降低敏感度）
        private static float _cameraDistance = 12f;  // 摄像机距离玩家的距离（增加距离）
        private static float _cameraFixedHeight = 2f;  // 摄像机固定高度
        
        [StartupSystem]
        public static void CreateTestEntities()
        {
            // Initialize logging system
            InitializeLogging();
            Log("=== 🚀 Starting Vampire Survivor Game ====");
            Log($"🎯 Creating {_enemyCount} enemy rats for enhanced gameplay...");

            // Register all meshes first
            RegisterAllMeshes();

            // Start timing the entity creation
            _creationStopwatch.Start();

            // === 创建玩家猴子实体 ===
            _playerId = EngineBindings.CreateEntity();
            var monkeyTransform = new Transform { position = new Vector3(0, 2, 0), scale = new Vector3(1) }; 
            EngineBindings.AddTransform(_playerId, monkeyTransform);
            var monkeyVelocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(_playerId, monkeyVelocity);

            // 添加Player组件，让猴子可以被PlayerSystem处理
            var player = new Player { isJumping = false, jumpForce = 8.0f };
            EngineBindings.AddPlayer(_playerId, player);

            // 添加猴子的Mesh和Material组件
            var monkeyMesh = new Mesh { modelId = 0 }; // 猴子是第一个模型
            EngineBindings.AddMesh(_playerId, monkeyMesh);
            var monkeyMaterial = new Material { color = new Vector3(0.8f, 0.6f, 0.4f), metallic = .1f, roughness = .9f, occlusion = .5f, emissive = new Vector3(.0f) }; // 棕色
            EngineBindings.AddMaterial(_playerId, monkeyMaterial);

            // 初始化全局玩家位置
            _playerPosition = monkeyTransform.position;
            Log($"🎯 初始化玩家位置: ({_playerPosition.X:F1}, {_playerPosition.Y:F1}, {_playerPosition.Z:F1})");
            Log($"🐵 Created PLAYER monkey entity with ID {_playerId}");

            // === 创建大量敌人鼠实体 ===
            for (int i = 0; i < _enemyCount; i++)
            {
                CreateEnemyRat(i);
                
                // Log progress every 1000 entities
                if ((i + 1) % 1000 == 0)
                {
                    Log($"📊 Created {i + 1}/{_enemyCount} enemy rats...");
                }
            }

            _creationStopwatch.Stop();
            Log($"✅ Enemy rat creation completed in {_creationStopwatch.ElapsedMilliseconds}ms");
            Log($"⚡ Average time per enemy: {_creationStopwatch.ElapsedMilliseconds / (double)_enemyCount:F3}ms");

            // === 创建摄像机实体 ===
            CreateGameCamera();

            // === 创建游戏统计实体 ===
            CreateGameStats();

            Log("=== 🎮 吸血鬼幸存者3D 游戏初始化完成 ===");
            Log("🐵 玩家: 无敌棕色猴子 - 使用WASD移动，空格跳跃");
            Log($"🐭 敌人: {_enemyCount} 只追踪玩家的老鼠 - 撞击玩家时会被销毁");
            Log("💥 无敌模式: 玩家碰到老鼠时老鼠会被销毁");
            Log("📈 高性能大规模实体管理测试");
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

        private static void RegisterAllMeshes()
        {
            Log("=== 🎯 Registering all meshes with the engine ===");

            // Define all meshes that the game will use (keeping all from Benchmark.cs for future reference)
            var meshDefinitions = new List<MeshDefinition>
            {
                new MeshDefinition { modelId = 0, modelPath = "models/blender-monkey/monkey.obj" },
                new MeshDefinition { modelId = 1, modelPath = "models/sci_sword/sword.gltf" },
                new MeshDefinition { modelId = 2, modelPath = "models/chest/Futuristic_Chest_1.gltf" },
                new MeshDefinition { modelId = 3, modelPath = "models/rat/rat_single.gltf" }
            };

            // Register each mesh with the native engine
            foreach (var meshDef in meshDefinitions)
            {
                try
                {
                    EngineBindings.RegisterMesh(meshDef.modelId, meshDef.modelPath);
                    Log($"Registered mesh ID {meshDef.modelId}: {meshDef.modelPath}");
                }
                catch (Exception ex)
                {
                    Log($"Failed to register mesh {meshDef.modelId}: {ex.Message}");
                }
            }

            Log("=== All meshes registered successfully ===");
        }

        // 玩家物理系统 - 只处理玩家的重力和碰撞
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        public static void PlayerPhysicsSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        {
            // 对玩家应用重力
            velocity.velocity.Y -= 9.81f * dt;

            // 更新玩家位置
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;

            // 地面碰撞检测
            if (transform.position.Y < 0)
            {
                transform.position.Y = 0;
                velocity.velocity.Y = 0;
            }

            // 更新全局玩家位置供吸血鬼AI使用
            _playerPosition = transform.position;

            // 玩家位置调试日志（降低频率）
            if (_testTimer > 2.0f)
            {
                Log($"🐵 Player - Position: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2})");
            }
        }

        // 老鼠物理系统 - 只处理老鼠的移动（不受重力影响）
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Mesh))]
        public static void VampirePhysicsSystem(float dt, ref Transform transform, ref Velocity velocity, ref Mesh mesh)
        {
            // 通过Model ID精确区分：0=玩家猴子，3=老鼠
            if (mesh.modelId == 0)
            {
                return; // 跳过玩家（猴子模型）
            }

            // 只处理老鼠（模型ID为3）
            if (mesh.modelId != 3)
            {
                return; // 跳过其他实体
            }

            // 老鼠不受重力影响，直接更新位置
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;

            // 保持在地面以上一定高度
            if (transform.position.Y < 0)
            {
                transform.position.Y = 0;
                velocity.velocity.Y = 0;
            }
        }

        // 老鼠AI系统 - 追踪玩家（无敌玩家版本）
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Mesh))]
        public static void VampireAISystem(float dt, ref Transform transform, ref Velocity velocity, ref Mesh mesh)
        {
            // 通过Model ID精确区分：0=玩家猴子，3=老鼠
            if (mesh.modelId == 0)
            {
                return; // 跳过玩家（猴子模型）
            }

            // 只处理老鼠（模型ID为3）
            if (mesh.modelId != 3)
            {
                return; // 跳过其他实体
            }

            float ratSpeed = 0.5f; // 统一的老鼠速度

            // 计算到玩家的距离和方向
            Vector3 toPlayer = _playerPosition - transform.position;
            float distanceToPlayer = toPlayer.Length();
            const float detectionRange = 15.0f; // 增大探测范围

            // 如果在探测范围内，追踪玩家
            if (distanceToPlayer <= detectionRange && distanceToPlayer > 0.1f) // 碰撞距离
            {
                // 标准化方向向量
                Vector3 direction = Vector3.Normalize(toPlayer);

                // 设置朝向玩家的速度（只在水平面移动，保持高度）
                velocity.velocity.X = direction.X * ratSpeed;
                velocity.velocity.Z = direction.Z * ratSpeed;
                velocity.velocity.Y = 0; // 保持高度恒定

                // 计算老鼠应该面向的角度（绕Y轴旋转，加180度让头部朝向玩家）
                float targetYaw = MathF.Atan2(direction.X, direction.Z) + MathF.PI;
                transform.rotation = new Vector3(0, targetYaw, 0);

                // 调试日志（降低频率）
                if (_testTimer > 3.0f)
                {
                    Log($"🐭 Rat at ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2}) " +
                        $"chasing invincible player at ({_playerPosition.X:F2}, {_playerPosition.Y:F2}, {_playerPosition.Z:F2}), distance: {distanceToPlayer:F2}");
                }
            }
            else if (distanceToPlayer <= 0.1f)
            {
                // 老鼠撞到无敌玩家 - 标记销毁！
                Log($"💥 Rat destroyed by invincible player! Distance: {distanceToPlayer:F2}");
                Log($"🐭 Player position: ({_playerPosition.X:F2}, {_playerPosition.Y:F2}, {_playerPosition.Z:F2})");
                Log($"🐭 Rat position: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2})");
                
                // 增加击杀数
                _killCount++;
                
                // 将变换移动到很远的地方，让它在下一帧被处理
                transform.position = new Vector3(99999f, -99999f, 99999f);
                velocity.velocity = Vector3.Zero;
                
                // 缩放为0，使其不可见
                transform.scale = Vector3.Zero;
            }
            else
            {
                // 超出探测范围，停止移动
                velocity.velocity = Vector3.Zero;
                if (_testTimer > 5.0f)
                {
                    Log($"🐭 Rat out of range, distance: {distanceToPlayer:F2}");
                }
            }
        }

        // 玩家控制系统 - 处理输入和跳跃（无敌版本）
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Player))]
        public static void PlayerSystem(float dt, ref Transform transform, ref Velocity velocity, ref Player player)
        {
            // 无敌玩家，不需要游戏结束检查
            
            // 使用新的游戏输入系统 - 简洁可靠的按键检测
            bool spaceJustPressed = EngineBindings.IsKeyJustPressed(Keys.GLFW_KEY_SPACE);
            bool isOnGround = transform.position.Y <= 0.1f;

            // 移动输入检测
            bool leftPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_A);
            bool rightPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_D);
            bool upPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_W);
            bool downPressed = EngineBindings.IsKeyPressed(Keys.GLFW_KEY_S);

            // 水平移动速度
            const float moveSpeed = 5.0f;
            float horizontalInput = 0.0f;
            float verticalInput = 0.0f;

            if (leftPressed) horizontalInput -= 1.0f;
            if (rightPressed) horizontalInput += 1.0f;
            if (upPressed) verticalInput -= 1.0f;
            if (downPressed) verticalInput += 1.0f;

            // 应用水平移动（不影响Y方向的速度，保持重力和跳跃的完整性）
            velocity.velocity.X = horizontalInput * moveSpeed;
            velocity.velocity.Z = verticalInput * moveSpeed;

            // 跳跃逻辑：只有在按下瞬间且在地面时才跳跃
            if (spaceJustPressed && isOnGround)
            {
                velocity.velocity.Y = 8.0f; // 跳跃力度
                Log("🐵 Player jumped!");
            }

            // 调试：输入状态（降低频率）
            if (_testTimer > 3.0f)
            {
                Log($"🎮 Input - Move: ({horizontalInput:F1}, {verticalInput:F1}), Jump: {spaceJustPressed}, Ground: {isOnGround}");
                _testTimer = 0;
            }
            _testTimer += dt;
        }


        //摄像机系统 - 鼠标自由操控的第三人称摄像机
        [UpdateSystem]
        [Query(typeof(Transform), typeof(iCamera))]
        public static void CameraSystem(float dt, ref Transform transform, ref iCamera camera)
        {
            // 获取鼠标移动量
            EngineBindings.GetMouseDelta(out float mouseDx, out float mouseDy);
            
            // 只有当鼠标实际移动时才更新角度
            if (MathF.Abs(mouseDx) > 1f || MathF.Abs(mouseDy) > 1f)
            {
                // 如果鼠标移动量过大，进行限制（防止疯狂移动）
                mouseDx = MathF.Max(-10f, MathF.Min(10f, mouseDx));
                mouseDy = MathF.Max(-10f, MathF.Min(10f, mouseDy));
                
                // 更新摄像机角度
                _cameraYaw += mouseDx * _mouseSensitivity;
                _cameraPitch += mouseDy * _mouseSensitivity;
                
                // 限制俯仰角度范围 (-89度到89度)
                _cameraPitch = MathF.Max(-1.55f, MathF.Min(1.55f, _cameraPitch));
            }
            
            // 计算摄像机在水平面上的位置（使用固定高度）
            float cosYaw = MathF.Cos(_cameraYaw);
            float sinYaw = MathF.Sin(_cameraYaw);
            float cosPitch = MathF.Cos(_cameraPitch);
            float sinPitch = MathF.Sin(_cameraPitch);
            
            // 计算摄像机相对于玩家的水平偏移量
            Vector3 horizontalOffset = new Vector3(
                _cameraDistance * cosPitch * sinYaw,    // X轴偏移
                0,                                      // 不使用Y轴偏移
                _cameraDistance * cosPitch * cosYaw     // Z轴偏移
            );
            
            // 设置摄像机位置（使用玩家的X、Z坐标，但固定高度）
            transform.position = new Vector3(
                _playerPosition.X + horizontalOffset.X,
                _cameraFixedHeight,  // 固定高度，不跟随玩家
                _playerPosition.Z + horizontalOffset.Z
            );
            
            // 摄像机始终朝向玩家的实际位置
            Vector3 directionToPlayer = _playerPosition - transform.position;
            float distanceToPlayer = directionToPlayer.Length();
            
            // 避免除零错误
            if (distanceToPlayer > 0.001f)
            {
                directionToPlayer = Vector3.Normalize(directionToPlayer);
                
                // 转换为欧拉角
                float lookYaw = MathF.Atan2(directionToPlayer.X, directionToPlayer.Z);
                float lookPitch = MathF.Asin(directionToPlayer.Y);
                
                // 限制俯仰角度范围，避免过于极端的角度
                const float maxPitchUp = 0.7f;    // 约40度向上
                const float maxPitchDown = -0.9f;  // 约52度向下
                lookPitch = MathF.Max(maxPitchDown, MathF.Min(maxPitchUp, lookPitch));
                
                // 设置摄像机旋转，让玩家始终在屏幕中央
                transform.rotation = new Vector3(lookYaw, lookPitch, 0);
            }
            
            // 调试信息（降低频率）
            if (_testTimer > 4.0f)
            {
                Log($"📷 Player: ({_playerPosition.X:F1}, {_playerPosition.Y:F1}, {_playerPosition.Z:F1})");
                Log($"📷 Camera: ({transform.position.X:F1}, {transform.position.Y:F1}, {transform.position.Z:F1})");
                Log($"📷 Angles - Yaw: {_cameraYaw:F2}, Pitch: {_cameraPitch:F2}");
                Log($"📷 Mouse Delta: ({mouseDx:F3}, {mouseDy:F3})");
            }
        }



        private static void CreateEnemyRat(int index)
        {
            // Create enemy rat entity
            uint entityId = EngineBindings.CreateEntity();
            _vampireIds.Add(entityId);

            // Position entities randomly in a disk pattern
            var random = new Random(index + 42); // Use index as seed for reproducible randomness
            
            // Generate random point in unit disk using rejection sampling
            float x, z;
            do
            {
                x = (float)random.NextDouble() * 2.0f - 1.0f; // [-1, 1]
                z = (float)random.NextDouble() * 2.0f - 1.0f; // [-1, 1]
            } while (x * x + z * z > 1.0f); // Reject points outside unit circle
            
            // Scale to desired spawn radius
            x *= _enemySpawnRadius;
            z *= _enemySpawnRadius;
            float y = 0;

            // Add Transform component
            var transform = new Transform 
            { 
                position = new Vector3(x, y, z), 
                scale = new Vector3(0.5f), 
                rotation = new Vector3(0, 0, 0) 
            };

            EngineBindings.AddTransform(entityId, transform);

            // Add Velocity component for movement
            var velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(entityId, velocity);

            // Add rat mesh (modelId = 3)
            var mesh = new Mesh { modelId = 3 };
            EngineBindings.AddMesh(entityId, mesh);

            // Add Material component with rat-like colors
            var material = CreateRatMaterial(index);
            EngineBindings.AddMaterial(entityId, material);

            // Record rat attributes
            _vampireSpeeds[entityId] = 0.5f;
        }

        private static Material CreateRatMaterial(int index)
        {
            var random = new Random(index);
            
            // Create brownish/grayish rat colors with some variation
            return new Material 
            { 
                color = new Vector3(0.4f, 0.3f, 0.2f) + new Vector3(
                    (float)random.NextDouble() * 0.3f - 0.15f,
                    (float)random.NextDouble() * 0.3f - 0.15f,
                    (float)random.NextDouble() * 0.3f - 0.15f
                ),
                metallic = 0.1f,
                roughness = 0.8f,
                occlusion = 0.6f,
                emissive = new Vector3(0.0f)
            };
        }

        private static void CreateGameCamera()
        {
            // === 创建摄像机实体 ===
            _cameraId = EngineBindings.CreateEntity();
            EngineBindings.AddCamera(_cameraId, new iCamera 
            { 
                fov = 60.0f, 
                nearPlane = 0.1f, 
                farPlane = 1000.0f 
            });
            
            // 初始摄像机位置
            EngineBindings.AddTransform(_cameraId, new Transform 
            { 
                position = new Vector3(0, 10, -15),
                rotation = new Vector3(0, 0, 0),
                scale = new Vector3(1) 
            });

            Log("📹 Created game camera");
        }

        private static void CreateGameStats()
        {
            // === 创建游戏统计实体 ===
            _gameStatsId = EngineBindings.CreateEntity();
            
            // 初始化游戏统计数据
            var gameStats = new GameStats
            {
                killCount = 0,
                gameTime = 0f
            };
            
            EngineBindings.AddGameStats(_gameStatsId, gameStats);
            
            // 记录游戏开始时间
            _gameStartTime = Environment.TickCount / 1000.0f;
            
            Log("📊 Created game statistics entity");
        }

        // 游戏统计系统 - 更新游戏时间和击杀数
        [UpdateSystem]
        [Query(typeof(GameStats))]
        public static void GameStatsSystem(float dt, ref GameStats gameStats)
        {
            // 更新游戏时间
            gameStats.gameTime = (Environment.TickCount / 1000.0f) - _gameStartTime;
            
            // 更新击杀数
            gameStats.killCount = _killCount;
        }

        // 清理系统 - 清理被销毁的老鼠
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Mesh))]
        public static void CleanupSystem(float dt, ref Transform transform, ref Mesh mesh)
        {
            // 只处理老鼠（模型ID为3）
            if (mesh.modelId != 3)
            {
                return;
            }

            // 如果老鼠被移动到很远的地方，说明它已经被"杀死"
            if (transform.position.X > 99990f || transform.position.Y < -99990f)
            {
                // 进一步缩小缩放，确保不可见
                transform.scale = Vector3.Zero;
            }
        }

        private class MeshDefinition
        {
            public int modelId;
            public string modelPath;
        }

    }
#endif
}

