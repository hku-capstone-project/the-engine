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
        private static float _testTimer = 0;  // 用于降低日志频率
        private static StreamWriter _logWriter = null;

        // 吸血鬼幸存者游戏变量
        private static uint _playerId = 0;  // 玩家实体ID

        private static uint _cameraId = 0;  // 摄像机实体ID

        private static List<uint> _vampireIds = new List<uint>();  // 吸血鬼实体ID列表
        private static Vector3 _playerPosition = Vector3.Zero;  // 玩家位置（全局共享）
        private static Dictionary<uint, float> _vampireSpeeds = new Dictionary<uint, float>();  // 吸血鬼移动速度

        [StartupSystem]
        public static void CreateTestEntities()
        {
            // Initialize logging system
            InitializeLogging();

            // Register all meshes first
            RegisterAllMeshes();


            // === 创建玩家猴子实体 ===
            _playerId = EngineBindings.CreateEntity();
            var monkeyTransform = new Transform { position = new Vector3(0, 1, 0), scale = new Vector3(1) }; // 修正Y坐标为1，添加缩放
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

            Log($"🐵 Created PLAYER monkey entity with ID {_playerId}");

            // === 创建吸血鬼剑实体1 ===
            uint vampire1Id = EngineBindings.CreateEntity();
            var vampire1Transform = new Transform { position = new Vector3(5, 1, 5), scale = new Vector3(10) }; // 修正Y坐标为1
            EngineBindings.AddTransform(vampire1Id, vampire1Transform);
            var vampire1Velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(vampire1Id, vampire1Velocity);

            // 添加剑的Mesh和Material组件
            var vampire1Mesh = new Mesh { modelId = 1 }; // 剑模型
            EngineBindings.AddMesh(vampire1Id, vampire1Mesh);
            var vampire1Material = new Material { color = new Vector3(0.8f, 0.1f, 0.1f), metallic = .9f, roughness = .1f, occlusion = .5f, emissive = new Vector3(.10f) }; // 血红色
            EngineBindings.AddMaterial(vampire1Id, vampire1Material);

            // 记录吸血鬼属性
            _vampireIds.Add(vampire1Id);
            _vampireSpeeds[vampire1Id] = 0.5f;  // 移动速度
            Log($"🧛‍♀️ Created VAMPIRE 1 entity with ID {vampire1Id}");

            // === 创建吸血鬼剑实体2 ===
            uint vampire2Id = EngineBindings.CreateEntity();
            var vampire2Transform = new Transform { position = new Vector3(-4, 0, -4), scale = new Vector3(5) }; // 修正Y坐标为1
            EngineBindings.AddTransform(vampire2Id, vampire2Transform);
            var vampire2Velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(vampire2Id, vampire2Velocity);

            // 添加剑的Mesh和Material组件
            var vampire2Mesh = new Mesh { modelId = 2 }; // 剑模型
            EngineBindings.AddMesh(vampire2Id, vampire2Mesh);
            var vampire2Material = new Material { color = new Vector3(0.6f, 0.0f, 0.6f) }; // 紫红色
            EngineBindings.AddMaterial(vampire2Id, vampire2Material);

            _cameraId = EngineBindings.CreateEntity();
            EngineBindings.AddCamera(_cameraId, new iCamera { fov = 60.0f, nearPlane = 0.1f, farPlane = 1000.0f });
            EngineBindings.AddTransform(_cameraId, new Transform { position = new Vector3(0, 0, 0), rotation = new Vector3(0), scale = new Vector3(.05f) });
            EngineBindings.AddVelocity(_cameraId, new Velocity { velocity = new Vector3(0, 0, 0) });

            // 记录吸血鬼属性
            _vampireIds.Add(vampire2Id);
            _vampireSpeeds[vampire2Id] = 0.5f;  // 移动速度
            Log($"🧛‍♀️ Created VAMPIRE 2 entity with ID {vampire2Id}");

            Log("=== 🎮 吸血鬼幸存者3D 游戏初始化完成 ===");
            Log("🐵 玩家: 棕色猴子 - 使用WASD移动，空格跳跃");
            Log("🧛‍♀️ 吸血鬼1: 血红色剑 - 会慢慢追踪玩家");
            Log("🧛‍♀️ 吸血鬼2: 紫红色剑 - 会慢慢追踪玩家");
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

            // Define all meshes that the game will use
            var meshDefinitions = new List<MeshDefinition>
            {
                new MeshDefinition { modelId = 0, modelPath = "models/blender-monkey/monkey.obj" },
                new MeshDefinition { modelId = 1, modelPath = "models/sci_sword/sword.gltf" },
                new MeshDefinition { modelId = 2, modelPath = "models/sci_sword/sword.gltf" },
                new MeshDefinition { modelId = 3, modelPath = "models/chest/Futuristic_Chest_1.gltf" }
            };

            // Register each mesh with the native engine
            foreach (var meshDef in meshDefinitions)
            {
                EngineBindings.RegisterMesh(meshDef.modelId, meshDef.modelPath);
                Log($"📦 Registered mesh ID {meshDef.modelId}: {meshDef.modelPath}");
            }

            Log("=== ✅ All meshes registered successfully ===");
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

        // 吸血鬼物理系统 - 只处理吸血鬼的移动（不受重力影响）
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Mesh))]
        public static void VampirePhysicsSystem(float dt, ref Transform transform, ref Velocity velocity, ref Mesh mesh)
        {
            // 通过Model ID精确区分：0=玩家猴子，1&2=吸血鬼剑
            if (mesh.modelId == 0)
            {
                return; // 跳过玩家（猴子模型）
            }

            // 只处理吸血鬼（剑模型ID为1或2）
            if (mesh.modelId != 1 && mesh.modelId != 2)
            {
                return; // 跳过其他实体
            }

            // 吸血鬼不受重力影响，直接更新位置
            transform.position.X += velocity.velocity.X * dt;
            transform.position.Y += velocity.velocity.Y * dt;
            transform.position.Z += velocity.velocity.Z * dt;

            // 保持在地面以上一定高度
            if (transform.position.Y < 1.0f)
            {
                transform.position.Y = 1.0f;
                velocity.velocity.Y = 0;
            }
        }

        // 吸血鬼AI系统 - 追踪玩家
        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity), typeof(Mesh))]
        public static void VampireAISystem(float dt, ref Transform transform, ref Velocity velocity, ref Mesh mesh)
        {
            // 通过Model ID精确区分：0=玩家猴子，1&2=吸血鬼剑
            if (mesh.modelId == 0)
            {
                return; // 跳过玩家（猴子模型）
            }

            // 只处理吸血鬼（剑模型ID为1或2）
            if (mesh.modelId != 1 && mesh.modelId != 2)
            {
                return; // 跳过其他实体
            }

            float vampireSpeed = 0.5f; // 统一的吸血鬼速度

            // 确保玩家位置已更新
            if (_playerPosition == Vector3.Zero)
            {
                _playerPosition = new Vector3(0, 1, 0); // 默认玩家位置
            }

            // 计算到玩家的距离和方向
            Vector3 toPlayer = _playerPosition - transform.position;
            float distanceToPlayer = toPlayer.Length();
            const float detectionRange = 15.0f; // 增大探测范围

            // 如果在探测范围内，追踪玩家
            if (distanceToPlayer <= detectionRange && distanceToPlayer > 0.5f) // 避免太近时抖动
            {
                // 标准化方向向量
                Vector3 direction = Vector3.Normalize(toPlayer);

                // 设置朝向玩家的速度（只在水平面移动，保持高度）
                velocity.velocity.X = direction.X * vampireSpeed;
                velocity.velocity.Z = direction.Z * vampireSpeed;
                velocity.velocity.Y = 0; // 保持高度恒定

                // 调试日志（降低频率）
                if (_testTimer > 3.0f)
                {
                    Log($"🧛‍♀️ Vampire at ({transform.position.X:F2}, {transform.position.Z:F2}) " +
                        $"chasing player at ({_playerPosition.X:F2}, {_playerPosition.Z:F2}), distance: {distanceToPlayer:F2}");
                }
            }
            else if (distanceToPlayer <= 0.5f)
            {
                // 太近了，停止移动
                velocity.velocity = Vector3.Zero;
                if (_testTimer > 3.0f)
                {
                    Log($"🧛‍♀️ Vampire reached player! Game over condition could trigger here.");
                }
            }
            else
            {
                // 超出探测范围，停止移动
                velocity.velocity = Vector3.Zero;
                if (_testTimer > 5.0f)
                {
                    Log($"🧛‍♀️ Vampire out of range, distance: {distanceToPlayer:F2}");
                }
            }
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


        //摄像机系统 - 处理摄像机位置和视角[UpdateSystem]
        [Query(typeof(Transform), typeof(iCamera))]
        public static void CameraSystem(float dt, ref Transform transform, ref iCamera camera)
        {


            Log($"📷 CameraSystem - PlayerPosition: ({_playerPosition.X:F2}, {_playerPosition.Y:F2}, {_playerPosition.Z:F2})");
            Log($"📷 CameraSystem - CameraPosition: ({transform.position.X:F2}, {transform.position.Y:F2}, {transform.position.Z:F2})");


            // 设置摄像机位置在玩家上方和后方
            float height = 5f; // 摄像机高度
            float distance = 10f; // 摄像机与玩家的水平距离
            float pitchAngle = MathF.PI / 6; // 俯仰角，30度（可调整）

            // 计算摄像机相对于玩家的偏移
            Vector3 offset = new Vector3(0, height, -distance);
            transform.position = _playerPosition + offset;

            // 计算摄像机朝向玩家的方向
            Vector3 directionToPlayer = _playerPosition - transform.position;
            directionToPlayer = Vector3.Normalize(directionToPlayer);

            // 计算旋转角度（Y轴旋转：偏航，X轴旋转：俯仰）
            float yaw = MathF.Atan2(directionToPlayer.X, directionToPlayer.Z); // 水平旋转
            float pitch = MathF.Asin(directionToPlayer.Y); // 垂直旋转（俯仰）

            // 应用旋转（俯仰角可手动调整以固定视角）
            transform.rotation = new Vector3(pitchAngle, yaw, 0); // 固定俯仰角，保持Y轴旋转
        }

        [UpdateSystem]
        [Query(typeof(iCamera))]
        public static void CameraSystemTest(float dt, ref iCamera camera)
        {


            if (_testTimer > 1.0f)
            {
                Log($"📷 CameraSystem - PlayerPosition: ({_playerPosition.X:F2}, {_playerPosition.Y:F2}, {_playerPosition.Z:F2})");
                _testTimer = 0;
            }
            _testTimer += 1.0f;

    
        }


    }
}

