#include "CameraInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void CameraInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
    auto const &ipos   = tomlConfigReader->getConfig<std::array<float, 3>>("Camera.initPosition");
    initPosition       = glm::vec3(ipos.at(0), ipos.at(1), ipos.at(2));
    initYaw            = tomlConfigReader->getConfig<float>("Camera.initYaw");
    initPitch          = tomlConfigReader->getConfig<float>("Camera.initPitch");
    vFov               = tomlConfigReader->getConfig<float>("Camera.vFov");
    movementSpeed      = tomlConfigReader->getConfig<float>("Camera.movementSpeed");
    movementSpeedBoost = tomlConfigReader->getConfig<float>("Camera.movementSpeedBoost");
    mouseSensitivity   = tomlConfigReader->getConfig<float>("Camera.mouseSensitivity");
}
