#include "ConfigContainer.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

#include "sub-config/ApplicationInfo.hpp"
#include "sub-config/CameraInfo.hpp"
#include "sub-config/DebugInfo.hpp"
#include "sub-config/ImguiManagerInfo.hpp"
#include "sub-config/RendererInfo.hpp"

ConfigContainer::ConfigContainer(Logger *logger) : _logger(logger) {
    applicationInfo  = std::make_unique<ApplicationInfo>();
    cameraInfo       = std::make_unique<CameraInfo>();
    debugInfo        = std::make_unique<DebugInfo>();
    imguiManagerInfo = std::make_unique<ImguiManagerInfo>();

    _loadConfig();
}

ConfigContainer::~ConfigContainer() = default;

void ConfigContainer::_loadConfig() {
    TomlConfigReader tomlConfigReader{_logger};

    applicationInfo->loadConfig(&tomlConfigReader);
    cameraInfo->loadConfig(&tomlConfigReader);
    debugInfo->loadConfig(&tomlConfigReader);
    imguiManagerInfo->loadConfig(&tomlConfigReader);
    RendererInfo->loadConfig(&tomlConfigReader);
}
