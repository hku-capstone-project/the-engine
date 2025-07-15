using System;
using System.IO;
using System.Collections.Generic;
using System.Numerics;
using System.Diagnostics;

namespace Game
{
#if Benchmark
    public static class BenchmarkSystems
    {
        private static float _testTimer = 0;  // 用于降低日志频率
        private static StreamWriter _logWriter = null;
        
        // 基准测试变量
        private static List<uint> _benchmarkEntityIds = new List<uint>();
        private static uint _cameraId = 0;
        private static int _entityCount = 10000;
        private static float _diskRadius = 10.0f;  // Configurable disk radius for entity placement
        private static Stopwatch _creationStopwatch = new Stopwatch();
        private static Stopwatch _frameStopwatch = new Stopwatch();
        
        // 摄像机控制变量
        private static float _cameraYaw = 0f;
        private static float _cameraPitch = 0f;
        private static float _mouseSensitivity = 0.0005f;
        private static float _cameraDistance = 50f;  // 更远的距离来看到更多实体
        private static float _cameraFixedHeight = 30f;  // 更高的高度
        private static Vector3 _cameraFocusPoint = Vector3.Zero;  // 摄像机焦点

        [StartupSystem]
        public static void CreateBenchmarkEntities()
        {
            // Initialize logging system
            InitializeLogging();
            Log("=== 🚀 Starting Benchmark System ===");
            Log($"🎯 Creating {_entityCount} entities for performance testing...");

            // Register all meshes first
            RegisterAllMeshes();

            // Start timing the entity creation
            _creationStopwatch.Start();

            // Create benchmark entities
            for (int i = 0; i < _entityCount; i++)
            {
                CreateBenchmarkEntity(i);
                
                // Log progress every 1000 entities
                if ((i + 1) % 1000 == 0)
                {
                    Log($"📊 Created {i + 1}/{_entityCount} entities...");
                }
            }

            _creationStopwatch.Stop();
            Log($"✅ Entity creation completed in {_creationStopwatch.ElapsedMilliseconds}ms");
            Log($"⚡ Average time per entity: {_creationStopwatch.ElapsedMilliseconds / (double)_entityCount:F3}ms");

            // Create camera for viewing
            CreateBenchmarkCamera();

            // Start frame timing
            _frameStopwatch.Start();

            Log("=== 🎮 Benchmark System Ready ===");
            Log($"📈 Monitoring performance with {_entityCount} entities");
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
                string logPath = Path.Combine(logsDir, $"benchmark_log_{timestamp}.txt");
                _logWriter = new StreamWriter(logPath, true);
                _logWriter.AutoFlush = true;

                Console.WriteLine($"=== Benchmark Logging System Initialized Successfully ===");
                Console.WriteLine($"Log file path: {logPath}");
                _logWriter.WriteLine($"=== Benchmark Logging System Initialized Successfully ===");
                _logWriter.WriteLine($"Log file path: {logPath}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Benchmark logging system initialization failed: {ex.Message}");
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
                Console.WriteLine($"Benchmark log write failed: {ex.Message}");
            }
        }

        private static void RegisterAllMeshes()
        {
            Log("=== 🎯 Registering all meshes with the engine ===");

            // Define all meshes that the benchmark will use
            var meshDefinitions = new List<MeshDefinition>
            {
                new MeshDefinition { modelId = 0, modelPath = "models/blender-monkey/monkey.obj" },
                new MeshDefinition { modelId = 1, modelPath = "models/sci_sword/sword.gltf" },
                new MeshDefinition { modelId = 2, modelPath = "models/chest/Futuristic_Chest_1.gltf" }
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

        private static void CreateBenchmarkEntity(int index)
        {
            // Create entity
            uint entityId = EngineBindings.CreateEntity();
            _benchmarkEntityIds.Add(entityId);

            // Position entities randomly in a disk pattern
            var random = new Random(index + 42); // Use index as seed for reproducible randomness
            
            // Generate random point in unit disk using rejection sampling
            float x, z;
            do
            {
                x = (float)random.NextDouble() * 2.0f - 1.0f; // [-1, 1]
                z = (float)random.NextDouble() * 2.0f - 1.0f; // [-1, 1]
            } while (x * x + z * z > 1.0f); // Reject points outside unit circle
            
            // Scale to desired disk radius
            x *= _diskRadius;
            z *= _diskRadius;
            float y = 0;

            // Add Transform component
            var transform = new Transform 
            { 
                position = new Vector3(x, y, z), 
                scale = new Vector3(0.5f), 
                rotation = new Vector3(0, 0, 0) 
            };
            
            Log($"Adding transform: {transform.position}, {transform.scale}, {transform.rotation}");

            EngineBindings.AddTransform(entityId, transform);

            // Add Velocity component for potential movement
            var velocity = new Velocity { velocity = new Vector3(0, 0, 0) };
            EngineBindings.AddVelocity(entityId, velocity);

            int modelId = 2;
            var mesh = new Mesh { modelId = modelId };
            EngineBindings.AddMesh(entityId, mesh);

            // Add Material component with different colors based on model
            var material = CreateMaterialForModel(modelId, index);
            EngineBindings.AddMaterial(entityId, material);
        }

        private static Material CreateMaterialForModel(int modelId, int index)
        {
            var random = new Random(index);
            
            switch (modelId)
            {
                case 0: // Monkey - brownish colors
                    return new Material 
                    { 
                        color = new Vector3(0.8f, 0.6f, 0.4f) + new Vector3(
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f
                        ),
                        metallic = 0.1f,
                        roughness = 0.9f,
                        occlusion = 0.5f,
                        emissive = new Vector3(0.0f)
                    };
                
                case 1: // Sword - red colors
                    return new Material 
                    { 
                        color = new Vector3(0.8f, 0.1f, 0.1f) + new Vector3(
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f
                        ),
                        metallic = 0.9f,
                        roughness = 0.1f,
                        occlusion = 0.5f,
                        emissive = new Vector3(0.1f, 0.0f, 0.0f)
                    };
                
                case 2: 
                    return new Material 
                    { 
                        color = new Vector3(0.8f, 0.7f, 0.2f) + new Vector3(
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f,
                            (float)random.NextDouble() * 0.2f - 0.1f
                        ),
                        metallic = 0.8f,
                        roughness = 0.2f,
                        occlusion = 0.5f,
                        emissive = new Vector3(0.05f, 0.05f, 0.0f)
                    };
                
                default:
                    return new Material 
                    { 
                        color = new Vector3(
                            (float)random.NextDouble(),
                            (float)random.NextDouble(),
                            (float)random.NextDouble()
                        ),
                        metallic = 0.5f,
                        roughness = 0.5f,
                        occlusion = 0.5f,
                        emissive = new Vector3(0.0f)
                    };
            }
        }

        private static void CreateBenchmarkCamera()
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
                position = new Vector3(0, _cameraFixedHeight, -_cameraDistance),
                rotation = new Vector3(0, 0, 0),
                scale = new Vector3(1) 
            });

            Log("📹 Created benchmark camera");
        }

        [UpdateSystem]
        [Query(typeof(Transform), typeof(Velocity))]
        public static void BenchmarkAnimationSystem(float deltaTime, UIntPtr transformPtr, UIntPtr velocityPtr)
        {
            // 仅在一个实体上执行动画逻辑
            unsafe
            {
                Transform* transform = (Transform*)transformPtr;
                Velocity* velocity = (Velocity*)velocityPtr;
                
                // 计算简单的波浪动画效果
                float time = _testTimer;
                float waveStrength = 0.5f;
                
                // 添加波浪动画到速度
                velocity->velocity = new Vector3(
                    MathF.Sin(time * 2.0f) * waveStrength,
                    MathF.Sin(time * 1.5f) * waveStrength * 0.5f,
                    MathF.Cos(time * 1.8f) * waveStrength
                );
            }
        }

        // 摄像机系统 - 鼠标自由操控
        [UpdateSystem]
        [Query(typeof(Transform), typeof(iCamera))]
        public static void BenchmarkCameraSystem(float dt, ref Transform transform, ref iCamera camera)
        {
            // 获取鼠标移动量
            EngineBindings.GetMouseDelta(out float mouseDx, out float mouseDy);
            
            // 只有当鼠标实际移动时才更新角度
            if (MathF.Abs(mouseDx) > 1f || MathF.Abs(mouseDy) > 1f)
            {
                // 如果鼠标移动量过大，进行限制
                mouseDx = MathF.Max(-10f, MathF.Min(10f, mouseDx));
                mouseDy = MathF.Max(-10f, MathF.Min(10f, mouseDy));
                
                // 更新摄像机角度
                _cameraYaw += mouseDx * _mouseSensitivity;
                _cameraPitch += mouseDy * _mouseSensitivity;
                
                // 限制俯仰角度范围
                _cameraPitch = MathF.Max(-1.55f, MathF.Min(1.55f, _cameraPitch));
            }
            
            // 计算摄像机位置
            float cosYaw = MathF.Cos(_cameraYaw);
            float sinYaw = MathF.Sin(_cameraYaw);
            float cosPitch = MathF.Cos(_cameraPitch);
            float sinPitch = MathF.Sin(_cameraPitch);
            
            // 计算摄像机相对于焦点的偏移量
            Vector3 offset = new Vector3(
                _cameraDistance * cosPitch * sinYaw,
                _cameraDistance * sinPitch,
                _cameraDistance * cosPitch * cosYaw
            );
            
            // 设置摄像机位置
            transform.position = _cameraFocusPoint + offset;
            
            // 摄像机朝向焦点
            Vector3 directionToFocus = _cameraFocusPoint - transform.position;
            directionToFocus = Vector3.Normalize(directionToFocus);
            
            // 转换为欧拉角
            float lookYaw = MathF.Atan2(directionToFocus.X, directionToFocus.Z);
            float lookPitch = MathF.Asin(-directionToFocus.Y);
            
            // 设置摄像机旋转
            transform.rotation = new Vector3(lookYaw, lookPitch, 0);
        }

        private class MeshDefinition
        {
            public int modelId;
            public string modelPath;
        }
    }
#endif
}