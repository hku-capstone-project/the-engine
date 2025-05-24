#pragma once

#include <memory>

struct ApplicationInfo;
struct AtmosInfo;
struct BrushInfo;
struct CameraInfo;
struct DebugInfo;
struct ImguiManagerInfo;
struct PostProcessingInfo;
struct ShadowMapCameraInfo;
struct RendererInfo;
struct TerrainInfo;
struct TracingInfo;

class Logger;

struct ConfigContainer {
    ConfigContainer(Logger *logger);
    ~ConfigContainer();

    // disable move and copy
    ConfigContainer(const ConfigContainer &)            = delete;
    ConfigContainer &operator=(const ConfigContainer &) = delete;
    ConfigContainer(ConfigContainer &&)                 = delete;
    ConfigContainer &operator=(ConfigContainer &&)      = delete;

    std::unique_ptr<ApplicationInfo> applicationInfo   = nullptr;
    std::unique_ptr<CameraInfo> cameraInfo             = nullptr;
    std::unique_ptr<DebugInfo> debugInfo               = nullptr;
    std::unique_ptr<ImguiManagerInfo> imguiManagerInfo = nullptr;
    std::unique_ptr<RendererInfo> RendererInfo         = nullptr;
    std::unique_ptr<TerrainInfo> terrainInfo           = nullptr;
    std::unique_ptr<TracingInfo> tracingInfo           = nullptr;

  private:
    Logger *_logger;

    void _loadConfig();
};
