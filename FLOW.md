# VulkanECS-Engine 运行流程说明

## 概述

本文档详细说明了从 `build.bat` 开始的 VulkanECS-Engine 完整运行流程。该引擎是一个高性能的 C++ 游戏引擎框架，采用数据导向的 ECS 架构（EnTT），支持 GPU 实例化的 Vulkan 渲染，主要脚本逻辑在 C++ 中实现，同时保留了与 C# 的互操作能力。

## 系统架构概览

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   C++ 脚本层    │    │  输入系统       │    │   渲染系统      │
│                 │    │                 │    │                 │
│ • 游戏逻辑      │◄──►│ • GLFW 输入     │◄──►│ • Vulkan 渲染   │
│ • ECS 系统      │    │ • 键盘/鼠标     │    │ • GPU 实例化    │
│ • 实体管理      │    │ • 状态缓存      │    │ • 着色器编译    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                ▲
                                │
                       ┌─────────────────┐
                       │   ECS 核心      │
                       │                 │
                       │ • EnTT 注册表   │
                       │ • 组件存储      │
                       │ • 系统调度      │
                       └─────────────────┘
```

## 第一部分：环境准备与依赖

### 前置要求

在运行 `build.bat` 之前，需要确保系统已安装以下工具：

1. **.NET 8.0.16 SDK**
   - 下载地址：[官方下载页面](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/sdk-8.0.410-windows-x64-installer)
   - 验证命令：`dotnet --list-sdks` 应显示 `8.0.410`
   - 验证命令：`dotnet --list-runtimes` 应显示 `Microsoft.NETCore.App 8.0.16`

2. **CMake 3.x**
   - 下载地址：[CMake 官网](https://cmake.org/download/)
   - 注意：避免使用 CMake 4.x（已移除对旧版本的兼容性支持）

3. **Ninja 构建系统**
   - 下载地址：[Ninja 发布页](https://github.com/ninja-build/ninja/releases)
   - 安装路径：`C:\Program Files\Ninja`
   - 需要添加到系统 PATH

4. **Ccache 编译缓存**
   - 下载地址：[Ccache 发布页](https://github.com/ccache/ccache/releases/tag/v4.10.2)
   - 安装路径：`C:\Program Files\ccache-<version>-windows-x86_64`
   - 需要添加到系统 PATH

5. **LLVM/Clang 18**
   - 下载地址：[LLVM 发布页](https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8)
   - **重要**：仅支持版本 18（由于 spdlog 兼容性问题）

6. **Vulkan SDK**
   - 下载地址：[Vulkan SDK](https://vulkan.lunarg.com/)
   - 用于支持 Vulkan 验证层

### 初始化项目

运行 `bootstrap.bat` 初始化项目：

```bash
bootstrap.bat
```

该脚本执行以下操作：
- 初始化 Git 子模块：`git submodule update --init --recursive`
- 引导 vcpkg 包管理器（如果尚未初始化）

## 第二部分：构建流程详解

### 运行构建命令

```bash
# 发布版本构建（默认）
build.bat

# 调试版本构建
build.bat --debug
```

### 构建流程分析

#### 1. 参数解析与环境设置

```batch
set BUILD_TYPE=release
set WITH_PORTABLE_RESOURCES=OFF

FOR %%a IN (%*) DO (
    if [%%a] == [--debug] set BUILD_TYPE=debug
)
```

- 默认构建类型为 `release`
- 检测 `--debug` 参数切换为调试构建
- 设置构建目录：`build/%BUILD_TYPE%/`

#### 2. C# 游戏项目发布（可选，当前未使用）

```batch
pushd game
dotnet publish -c Release -r win-x64 --self-contained false -o ../build/Game
```

**注意**：当前版本的引擎主要使用 C++ 实现游戏逻辑，虽然构建过程中会编译 C# 项目，但主要的脚本系统在 `src/app/Application.cpp` 中实现。C# 项目保留用于未来扩展和互操作性测试。

#### 3. CMake 配置

```batch
cmake --preset %BUILD_TYPE% ^
    -D CMAKE_TOOLCHAIN_FILE="dep/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -D VCPKG_MANIFEST_INSTALL=ON ^
    -D WITH_PORTABLE_RESOURCES=%WITH_PORTABLE_RESOURCES% ^
    -D DOTNET_HOSTING_DIR="dep/dotnet-runtime-8.0.16" ^
    -D CMAKE_MAKE_PROGRAM=Ninja
```

关键配置项：
- **vcpkg 工具链**：管理 C++ 依赖包
- **清单模式**：启用 vcpkg.json 自动安装
- **.NET 宿主目录**：指定 .NET 运行时位置
- **构建器**：使用 Ninja 加速构建

#### 4. C++ 项目构建

```batch
cmake --build %BINARY_DIR%
```

构建过程包括：
- 编译 C++ 源文件（使用 Clang++）
- 链接 Vulkan、GLFW、EnTT 等依赖库
- 生成主可执行文件 `run.exe`

#### 5. 后处理操作

- **复制编译命令数据库**：
  ```batch
  robocopy %BINARY_DIR% . compile_commands.json
  ```

- **复制 .NET 运行时 DLL**（为 C# 互操作预留）：
  ```batch
  robocopy "dep/dotnet-runtime-8.0.16/" "%PROJECT_EXECUTABLE_PATH%" *.dll
  ```

- **资源文件处理**（可选）：
  ```batch
  if %WITH_PORTABLE_RESOURCES%==ON (
      robocopy "resources/" "!RESOURCES_PATH!" /E /IS
  )
  ```

## 第三部分：应用程序启动流程

### 程序启动

构建完成后，`build.bat` 自动启动应用程序：

```batch
set ENGINE_ROOT=%CD%
start /wait /b /d "%PROJECT_EXECUTABLE_PATH%" run.exe
```

### 应用程序初始化序列

#### 1. 主函数入口 (`apps/main.cpp`)

```cpp
int main() {
    Logger logger{};
    {
        auto app = std::make_unique<Application>(&logger);
        app->run();
    }
    return EXIT_SUCCESS;
}
```

#### 2. Application 构造函数

按以下顺序初始化各个系统：

1. **Vulkan 应用上下文**
   ```cpp
   _appContext = std::make_unique<VulkanApplicationContext>();
   ```

2. **配置容器**
   ```cpp
   _configContainer = std::make_unique<ConfigContainer>(_logger);
   ```
   - 加载 `resources/configs/DefaultConfig.toml`
   - 解析应用程序设置（帧率限制、飞行帧数等）

3. **着色器编译器**
   ```cpp
   _shaderCompiler = std::make_unique<ShaderCompiler>(logger, ...);
   ```

4. **窗口系统**
   ```cpp
   _window = std::make_unique<Window>(WindowStyle::kMaximized, logger);
   ```
   - 创建 GLFW 窗口
   - 设置为最大化模式

5. **Vulkan 初始化**
   ```cpp
   _appContext->init(_logger, _window->getGlWindow(), &settings);
   ```

6. **ImGui 管理器**
   ```cpp
   _imguiManager = std::make_unique<ImguiManager>(...);
   ```

7. **渲染器**
   ```cpp
   _renderer = std::make_unique<Renderer>(...);
   ```

8. **脚本引擎初始化**
   ```cpp
   _initScriptEngine();
   ```

9. **模型管理器初始化**
   ```cpp
   _modelManager = std::make_unique<ModelManager>(_appContext, _logger);
   ```

#### 3. 系统初始化 (`_init()`)

1. **ImGui 初始化**
2. **同步对象创建**（信号量和栅栏）
3. **输入系统初始化**（Unity 风格的 InputSystem）
4. **键盘回调注册**
5. **ECS 实体创建**（猴子和汽车模型）
6. **控制模式说明输出**

### 模型管理系统

#### 模型注册位置

模型的注册和ID分配在 **`src/renderer/Renderer.cpp`** 的构造函数中进行：

```cpp
// 创建模型管理器
_modelManager = std::make_unique<ModelManager>(_appContext, _logger);

// 预加载猴子模型 (自动分配ID = 0)
const auto monkeyModelPath = kPathToResourceFolder + "models/blender-monkey/monkey.obj";
int32_t monkeyId = _modelManager->loadModel(monkeyModelPath);

// 预加载汽车模型 (自动分配ID = 1)
const auto carModelPath = kPathToResourceFolder + "models/car/car.obj";
int32_t carId = _modelManager->loadModel(carModelPath);
```

#### 模型ID分配机制

**ModelManager** (`src/utils/model-loader/ModelManager.cpp`) 负责管理模型ID和路径的绑定：

```cpp
int32_t ModelManager::loadModel(const std::string& modelPath) {
    auto model = std::make_unique<Model>(_appContext, _logger, modelPath);
    
    int32_t modelId = _nextModelId++;  // 自动递增分配ID
    _models[modelId] = std::move(model);
    
    _logger->info("ModelManager: Loaded model '{}' with ID {}", modelPath, modelId);
    return modelId;
}
```

#### 当前模型映射表

| 模型ID | 模型路径 | 用途 |
|--------|----------|------|
| 0 | `resources/models/blender-monkey/monkey.obj` | 猴子模型（可控制） |
| 1 | `resources/models/car/car.obj` | 汽车模型（静止/震动） |

#### 添加新模型的步骤

1. 将模型文件放入 `resources/models/` 目录
2. 在 `Renderer.cpp` 构造函数中添加加载代码：
   ```cpp
   const auto newModelPath = kPathToResourceFolder + "models/your-model/model.obj";
   int32_t newModelId = _modelManager->loadModel(newModelPath);
   ```
3. 在 `Application.cpp` 中创建使用该模型的实体：
   ```cpp
   auto entity = _scriptEngine->registry.create();
   Mesh mesh;
   mesh.modelId = newModelId;  // 使用返回的ID
   _scriptEngine->registry.emplace<Mesh>(entity, mesh);
   ```

### C++ 脚本引擎架构

#### 核心组件

1. **ECS 系统**（基于 EnTT）
   ```cpp
   _scriptEngine = std::make_unique<App>();
   ```
   - `App` 类包含 EnTT 注册表
   - 启动系统列表 (`startSystems`)
   - 更新系统列表 (`updateSystems`)

2. **输入系统集成**
   ```cpp
   InputSystem::Initialize();
   InputSystem::UpdateFrame(_window->getKeyboardInfo(), _window->getCursorInfo());
   ```
   - Unity 风格的输入 API
   - `GetKey()`, `GetKeyDown()`, `GetKeyUp()` 
   - 鼠标输入支持

3. **组件定义**（在 C++ 中）
   ```cpp
   struct Transform {
       glm::vec3 position = glm::vec3(0.0f);
   };
   
   struct Mesh {
       int modelId = 0;
   };
   ```

#### 控制模式系统

1. **随机移动模式**（默认，F1切换）
   - 猴子围绕汽车做圆周运动，带上下浮动效果
   - 汽车静止在场景中心 (0,0,0) 位置
   - 使用正弦波组合计算猴子位置

2. **脚本控制模式**（F2 切换）
   - 使用 `InputTestSystem::CreatePlayerControllerSystem()`
   - 只控制猴子（ModelID: 0），汽车不受影响
   - WASD/QZ 控制猴子移动，实时位置输出
   - 汽车在原点进行震动效果

## 第四部分：运行时执行

### 主循环 (`_mainLoop()`)

```cpp
while (glfwWindowShouldClose(_window->getGlWindow()) == 0) {
    glfwPollEvents();
    
    // 处理阻塞状态（窗口调整、着色器重新编译等）
    if (_blockStateBits != 0) { ... }
    
    // 计算帧时间
    auto deltaTime = calculateDeltaTime();
    
    // FPS 统计
    _fpsSink->addRecord(1.0F / deltaTimeInSec);
    
    // GUI 绘制
    _imguiManager->draw(_fpsSink.get());
    
    // 输入处理
    _renderer->processInput(deltaTimeInSec);
    
    // 绘制帧
    _drawFrame();
}
```

### 帧渲染流程 (`_drawFrame()`)

1. **同步等待**
   ```cpp
   vkWaitForFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame], ...);
   ```

2. **交换链图像获取**
   ```cpp
   vkAcquireNextImageKHR(_appContext->getDevice(), _appContext->getSwapchain(), ...);
   ```

3. **输入系统更新**
   ```cpp
   InputSystem::UpdateFrame(_window->getKeyboardInfo(), _window->getCursorInfo());
   ```

4. **脚本系统执行**
   ```cpp
   for (auto &s : _scriptEngine->updateSystems) {
       s(deltaTime);
   }
   ```

5. **实体位置更新**
   - **随机移动模式（F1）**：
     - 猴子（ModelID: 0）围绕汽车做圆周运动
     ```cpp
     float radius = 3.0f;
     transform.position.x = radius * std::cos(globalTime * speed);
     transform.position.y = 1.0f + 0.5f * std::sin(globalTime * 2.0f);  // 上下浮动
     transform.position.z = radius * std::sin(globalTime * speed);
     ```
     - 汽车（ModelID: 1）固定在原点
     ```cpp
     transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
     ```
   - **脚本控制模式（F2）**：
     - 猴子由 `InputTestSystem` 控制，支持 WASD 移动
     ```cpp
     if (Input::GetKey_W()) transform.position.z -= moveSpeed;  // 前进
     if (Input::GetKey_S()) transform.position.z += moveSpeed;  // 后退
     if (Input::GetKey_A()) transform.position.x -= moveSpeed;  // 左移
     if (Input::GetKey_D()) transform.position.x += moveSpeed;  // 右移
     ```
     - 汽车原地震动
     ```cpp
     float shake = 0.02f;
     transform.position.x = shake * std::sin(globalTime * 30.0f);
     transform.position.y = shake * std::cos(globalTime * 25.0f);
     transform.position.z = shake * std::sin(globalTime * 35.0f);
     ```

6. **渲染命令提交**
   ```cpp
   _renderer->drawFrame(currentFrame, imageIndex, modelMatrix);
   _imguiManager->recordCommandBuffer(currentFrame, imageIndex);
   ```

7. **GPU 提交与呈现**
   ```cpp
   vkQueueSubmit(_appContext->getGraphicsQueue(), ...);
   vkQueuePresentKHR(_appContext->getPresentQueue(), ...);
   ```

## 第五部分：用户交互

### 控制方式

#### 摄像机控制
- **方向键**：前进/后退/左移/右移
- **Space/Ctrl**：上升/下降
- **鼠标移动**：视角控制

#### 模式切换
- **F1**：随机移动模式（猴子围绕汽车转圈，汽车静止）
- **F2**：脚本控制模式（WASD 控制猴子，汽车震动）

#### 游戏对象控制（F2模式下）
- **WASD**：控制猴子移动
- **Q/Z**：控制猴子上升/下降
- **Space**：打印猴子当前位置

#### 系统控制
- **E**：切换鼠标捕获模式
- **ESC**：退出应用程序

### 渲染特性

1. **GPU 实例化渲染**
   - 单次绘制调用处理多个相同网格
   - 间接绘制命令
   - 计算着色器视锥体剔除

2. **现代 Vulkan 管线**
   - 描述符集管理
   - 同步原语（信号量、栅栏）
   - 多缓冲渲染

3. **资源管理**
   - 自动着色器重新编译
   - 模型/纹理加载
   - 配置热重载

## 构建输出结构

```
build/
├── debug/ 或 release/
│   ├── apps/
│   │   ├── run.exe          # 主可执行文件
│   │   ├── *.dll            # .NET 运行时库（预留）
│   │   └── resources/       # 资源文件（便携模式）
│   └── src/                 # 中间构建文件
└── Game/                    # C# 游戏程序集（预留，当前未使用）
    ├── Game.dll
    ├── Game.deps.json
    └── ...
```

## 核心脚本系统实现

### InputTestSystem 详解

`InputTestSystem` 是当前引擎中主要的游戏逻辑系统，位于 `src/dotnet/InputTestSystem.hpp`：

```cpp
class InputTestSystem {
public:
    static void CreatePlayerControllerSystem(App& app) {
        app.add_update_system([&app](float deltaTime) {
            auto view = app.registry.view<Transform, Mesh>();
            
            for (auto entity : view) {
                auto& transform = view.get<Transform>(entity);
                auto& mesh = view.get<Mesh>(entity);
                
                if (mesh.modelId == 0) {  // 控制猴子实体
                    float moveSpeed = 2.0f * deltaTime;
                    
                    if (Input::GetKey_W()) {
                        transform.position.z -= moveSpeed;  // 前进
                    }
                    // ... 其他方向控制
                }
            }
        });
    }
};
```

### 输入系统架构

引擎实现了类似 Unity 的输入系统：

```cpp
// 输入包装器 (src/dotnet/InputWrapper.hpp)
class Input {
public:
    static bool GetKey(int keyCode);
    static bool GetKeyDown(int keyCode);  // 按下瞬间
    static bool GetKeyUp(int keyCode);    // 释放瞬间
    static glm::vec2 GetMousePosition();
    
    // 便捷函数
    static bool GetKey_W() { return GetKey(KeyCode::W); }
    static bool GetKey_A() { return GetKey(KeyCode::A); }
    // ...
};
```

## 故障排除

### 常见构建错误

1. **vcpkg 未初始化**
   ```
   解决方案：运行 bootstrap.bat
   ```

2. **.NET SDK 版本不匹配**
   ```
   解决方案：安装 .NET 8.0.16 SDK
   ```

3. **Vulkan SDK 缺失**
   ```
   解决方案：安装最新 Vulkan SDK
   ```

4. **LLVM 版本冲突**
   ```
   解决方案：确保使用 LLVM 18.x
   ```

### 运行时错误

1. **Vulkan 验证层错误**
   ```
   检查：GPU 驱动程序版本
   检查：Vulkan SDK 安装
   ```

2. **输入系统错误**
   ```
   检查：GLFW 窗口初始化
   检查：键盘回调注册
   检查：InputSystem::Initialize() 调用
   ```

## 性能优化建议

1. **构建优化**
   - 使用 ccache 加速重新编译
   - 启用 LTO（链接时优化）
   - 使用发布模式构建

2. **运行时优化**
   - 启用帧率限制
   - 调整飞行帧数
   - 使用便携资源模式减少 I/O

## 总结

VulkanECS-Engine 的运行流程展现了现代游戏引擎的复杂性和精密设计：

1. **混合语言架构**：充分利用 C++ 的性能和 C# 的生产力
2. **现代渲染管线**：基于 Vulkan 的高性能 GPU 渲染
3. **数据导向设计**：ECS 架构提供高效的内存访问模式
4. **模块化设计**：清晰的系统边界和职责分离

从 `build.bat` 开始的整个流程涵盖了从源码编译到实时渲染的完整生命周期，为开发者提供了一个强大而灵活的 3D 游戏开发基础。 