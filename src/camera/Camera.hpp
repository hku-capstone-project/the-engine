#pragma once

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "window/CursorInfo.hpp"
#include "window/Window.hpp"

struct ConfigContainer;
class Logger;

glm::vec3 constexpr kWorldUp = {0.F, 1.F, 0.F};

class Camera {
  public:
    Camera(Window *window, Logger *logger, ConfigContainer *configContainer);
    ~Camera();

    // disable move and copy
    Camera(const Camera &)            = delete;
    Camera &operator=(const Camera &) = delete;
    Camera(Camera &&)                 = delete;
    Camera &operator=(Camera &&)      = delete;

    [[nodiscard]] glm::mat4 getViewMatrix() const {
        return glm::lookAt(_position, _position + _front, kWorldUp);
    }

    void processInput(double deltaTime);

    // processes input received from any keyboard-like input system. Accepts input
    // parameter in the form of camera defined ENUM (to abstract it from windowing
    // systems)
    void processKeyboard(double deltaTime);

    // processes input received from a mouse input system. Expects the offset
    // value in both the x and y direction.
    void handleMouseMovement(CursorMoveInfo const &mouseInfo);

    // processes input received from a mouse scroll-wheel event. Only requires
    // input on the vertical wheel-axis void processMouseScroll(float yoffset);
    // TODO: zNear and zFar should be configurable? useful?
    [[nodiscard]] glm::mat4 getProjectionMatrix() const;

    [[nodiscard]] glm::vec3 getPosition() const { return _position; }
    [[nodiscard]] glm::vec3 getFront() const { return _front; }
    [[nodiscard]] glm::vec3 getUp() const { return _up; }
    [[nodiscard]] glm::vec3 getRight() const { return _right; }
    // get fov
    [[nodiscard]] float getFov() const { return _fov; }
    // get near plane
    [[nodiscard]] float getNearPlane() const { return _nearPlane; }
    // get far plane
    [[nodiscard]] float getFarPlane() const { return _farPlane; }

    [[nodiscard]] float getAspectRatio() const { return aspectRatio; }
    [[nodiscard]] float getYaw() const { return _yaw; }
    [[nodiscard]] float getPitch() const { return _pitch; }

    // add setters for position,rotation, fov, near plane, far plane, yaw and pitch
    void setPosition(const glm::vec3 &position) {
        _position = position;
        _updateCameraVectors();
    }

    void setRotation(const glm::vec3 &rotation) {

        _rotation = rotation;
        // 更新相机向量
        _updateCameraVectors();
    }

    void setFov(float fov) { _fov = fov; }
    void setNearPlane(float nearPlane) { _nearPlane = nearPlane; }
    void setFarPlane(float farPlane) { _farPlane = farPlane; }

    void setAspectRatio(float ratio) { aspectRatio = ratio; }

    void setYaw(float yaw) {
        _yaw = yaw;
        _updateCameraVectors();
    }
    void setPitch(float pitch) {
        _pitch = pitch;
        _updateCameraVectors();
    }

  private:
    Logger *_logger;

    glm::vec3 _front = glm::vec3(0.F);
    glm::vec3 _up    = glm::vec3(0.F);
    glm::vec3 _right = glm::vec3(0.F);

    // window is owned by the Application class
    Window *_window;
    ConfigContainer *_configContainer;

    glm::vec3 _position{};
    glm::vec3 _rotation{};

    float _fov;
    float _nearPlane;
    float _farPlane;
    float aspectRatio = 16.F / 9.F; // default aspect ratio

    float _yaw{};
    float _pitch{};

    float _movementSpeedMultiplier = 1.F;

    // calculates the front vector from the Camera's (updated) Euler Angles
    void _updateCameraVectors();
    [[nodiscard]] bool canMove() const {
        return _window->getCursorState() == CursorState::kInvisible;
    }

    void _printInfo();
};
