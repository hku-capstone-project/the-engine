    #include "Camera.hpp"

    #include "config-container/ConfigContainer.hpp"
    #include "config-container/sub-config/CameraInfo.hpp"
    #include "utils/logger/Logger.hpp"
    #include "window/KeyboardInfo.hpp"



    #define GLFW_THUMB_KEY GLFW_KEY_LEFT_CONTROL

    Camera::Camera(Window *window, Logger *logger, ConfigContainer *configContainer)
        : _window(window), _logger(logger), _configContainer(configContainer) {
        _position = _configContainer->cameraInfo->initPosition;
        _rotation = glm::vec3{
            _configContainer->cameraInfo->initPitch, 
            _configContainer->cameraInfo->initYaw, 
            0.F // roll is not used
        };
        _fov          = _configContainer->cameraInfo->vFov;
        _updateCameraVectors();
    }

    Camera::~Camera() = default;

    glm::mat4 Camera::getProjectionMatrix() const {
        // right handed
        glm::mat4 projection = glm::perspective(
            glm::radians(_fov), 
            aspectRatio,
            _nearPlane, 
            _farPlane  
        );
        projection[1][1]      *= -1.0f;
        return projection;
    }

    void Camera::processInput(double deltaTime) { 
        // processKeyboard(deltaTime); 
    }

    void Camera::processKeyboard(double deltaTime) {
        // if (!canMove()) {
        //     return;
        // }

        // float velocity = _movementSpeedMultiplier * _configContainer->cameraInfo->movementSpeed *
        //                  static_cast<float>(deltaTime);

        // KeyboardInfo const &ki = _window->getKeyboardInfo();

        // if (ki.isKeyPressed(GLFW_KEY_W)) {
        //     _position += _front * velocity;
        // }
        // if (ki.isKeyPressed(GLFW_KEY_S)) {
        //     _position -= _front * velocity;
        // }
        // if (ki.isKeyPressed(GLFW_KEY_A)) {
        //     _position -= _right * velocity;
        // }
        // if (ki.isKeyPressed(GLFW_KEY_D)) {
        //     _position += _right * velocity;
        // }
        // if (ki.isKeyPressed(GLFW_KEY_SPACE)) {
        //     _position += kWorldUp * velocity;
        // }
        // if (ki.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
        //     _movementSpeedMultiplier = _configContainer->cameraInfo->movementSpeedBoost;
        // } else {
        //     _movementSpeedMultiplier = 1.F;
        // }
        // if (ki.isKeyPressed(GLFW_THUMB_KEY)) {
        //     _position -= kWorldUp * velocity;
        // }
    }

    void Camera::handleMouseMovement(CursorMoveInfo const &mouseInfo) {
        // if (!canMove()) {
        //     return;
        // }

        // float mouseDx = mouseInfo.dx;
        // float mouseDy = mouseInfo.dy;

        // mouseDx *= _configContainer->cameraInfo->mouseSensitivity;
        // mouseDy *= _configContainer->cameraInfo->mouseSensitivity;

        // _yaw -= mouseDx;
        // _pitch += mouseDy;

        // constexpr float cameraLim = 89.9F;
        // // make sure that when mPitch is out of bounds, screen doesn't get flipped
        // if (_pitch > cameraLim) {
        //     _pitch = cameraLim;
        // }
        // if (_pitch < -cameraLim) {
        //     _pitch = -cameraLim;
        // }

        // // update Front, Right and Up Vectors using the updated Euler angles
        // _updateCameraVectors();
    }

    void Camera::_updateCameraVectors() {
        // 假设 _rotation 的单位是度，转换为弧度
        float pitch = (_rotation.y);
        float yaw = (_rotation.x);
        // float roll = glm::radians(_rotation.z); // 如果需要 roll，可以启用

        // 计算 front 向量（与原函数一致）
        _front = glm::vec3{
            sin(yaw) * cos(pitch),
            sin(pitch),
            cos(yaw) * cos(pitch)
        };

        // 规范化 front 向量，确保长度为 1
        _front = glm::normalize(_front);

        // 计算 right 向量（使用世界坐标系的上向量 kWorldUp）
        _right = glm::normalize(glm::cross(_front, kWorldUp));

        // 计算 up 向量
        _up = glm::normalize(glm::cross(_right, _front));



    }

    void Camera::_printInfo() {
        _logger->info("Camera position: ({}, {}, {})", _position.x, _position.y, _position.z);
        _logger->info("Camera front: ({}, {}, {})", _front.x, _front.y, _front.z);
        _logger->info("Camera up: ({}, {}, {})", _up.x, _up.y, _up.z);
        _logger->info("Camera right: ({}, {}, {})", _right.x, _right.y, _right.z);
        _logger->info("Camera yaw: {}, pitch: {}", _yaw, _pitch);
    }
