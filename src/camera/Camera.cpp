#include "Camera.hpp"

#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/CameraInfo.hpp"
#include "utils/logger/Logger.hpp"
#include "window/KeyboardInfo.hpp"

glm::vec3 constexpr kWorldUp = {0.F, 1.F, 0.F};

#define GLFW_THUMB_KEY GLFW_KEY_LEFT_CONTROL

Camera::Camera(Window *window, Logger *logger, ConfigContainer *configContainer)
    : _window(window), _logger(logger), _configContainer(configContainer) {
    auto const &h = _configContainer->cameraInfo->initHeight;
    _position     = glm::vec3(0.0F, h, 0.0F);
    _yaw          = _configContainer->cameraInfo->initYaw;
    _pitch        = _configContainer->cameraInfo->initPitch;

    _updateCameraVectors();
}

Camera::~Camera() = default;

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
    glm::mat4 projection = glm::perspective(
        glm::radians(_configContainer->cameraInfo
                         ->vFov), // The vertical Field of View, in radians: the amount
                                  // of "zoom". Think "camera lens". Usually between
                                  // 90° (extra wide) and 30° (quite zoomed in)
        aspectRatio,
        zNear, // Near clipping plane. Keep as big as possible, or you'll get
               // precision issues.
        zFar   // Far clipping plane. Keep as little as possible.
    );
    return projection;
}

float Camera::getVFov() const { return _configContainer->cameraInfo->vFov; }

void Camera::processInput(double deltaTime) {
    // _printInfo();
    processKeyboard(deltaTime);
}

void Camera::processKeyboard(double deltaTime) {
    if (!canMove()) {
        return;
    }

    float velocity = _movementSpeedMultiplier * _configContainer->cameraInfo->movementSpeed *
                     static_cast<float>(deltaTime);

    KeyboardInfo const &ki = _window->getKeyboardInfo();

    if (ki.isKeyPressed(GLFW_KEY_W)) {
        _position += _front * velocity;
    }
    if (ki.isKeyPressed(GLFW_KEY_S)) {
        _position -= _front * velocity;
    }
    if (ki.isKeyPressed(GLFW_KEY_A)) {
        _position -= _right * velocity;
    }
    if (ki.isKeyPressed(GLFW_KEY_D)) {
        _position += _right * velocity;
    }
    if (ki.isKeyPressed(GLFW_KEY_SPACE)) {
        _position += kWorldUp * velocity;
    }
    if (ki.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        _movementSpeedMultiplier = _configContainer->cameraInfo->movementSpeedBoost;
    } else {
        _movementSpeedMultiplier = 1.F;
    }
    if (ki.isKeyPressed(GLFW_THUMB_KEY)) {
        _position -= kWorldUp * velocity;
    }
}

void Camera::handleMouseMovement(CursorMoveInfo const &mouseInfo) {
    if (!canMove()) {
        return;
    }

    float mouseDx = mouseInfo.dx;
    float mouseDy = mouseInfo.dy;

    mouseDx *= -_configContainer->cameraInfo->mouseSensitivity;
    mouseDy *= _configContainer->cameraInfo->mouseSensitivity;

    _yaw += mouseDx;
    _pitch += mouseDy;

    constexpr float cameraLim = 89.9F;
    // make sure that when mPitch is out of bounds, screen doesn't get flipped
    if (_pitch > cameraLim) {
        _pitch = cameraLim;
    }
    if (_pitch < -cameraLim) {
        _pitch = -cameraLim;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    _updateCameraVectors();
}

void Camera::_updateCameraVectors() {
    _front = {-sin(glm::radians(_yaw)) * cos(glm::radians(_pitch)), sin(glm::radians(_pitch)),
              -cos(glm::radians(_yaw)) * cos(glm::radians(_pitch))};
    // normalize the vectors, because their length gets closer to 0 the
    _right = glm::normalize(glm::cross(_front, kWorldUp));
    // more you look up or down which results in slower movement.
    _up = glm::cross(_right, _front);
}

void Camera::_printInfo() {
    _logger->info("Camera position: ({}, {}, {})", _position.x, _position.y, _position.z);
    _logger->info("Camera front: ({}, {}, {})", _front.x, _front.y, _front.z);
    _logger->info("Camera up: ({}, {}, {})", _up.x, _up.y, _up.z);
    _logger->info("Camera right: ({}, {}, {})", _right.x, _right.y, _right.z);
    _logger->info("Camera yaw: {}, pitch: {}", _yaw, _pitch);
}
