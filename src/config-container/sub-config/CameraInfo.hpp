#pragma once

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: keep

class TomlConfigReader;

struct CameraInfo {
    glm::vec3 initPosition{};
    float initYaw{};   // in euler angles
    float initPitch{}; // in euler angles
    float vFov{};
    float movementSpeed{};
    float movementSpeedBoost{};
    float mouseSensitivity{};

    void loadConfig(TomlConfigReader *tomlConfigReader);
};
