#pragma once

#include "../input/InputSystem.hpp"
#include <glm/glm.hpp>

/**
 * Input wrapper for script system
 * This provides a simple interface for scripts to access input state
 */
class Input {
public:
    // Key input functions
    static bool GetKey(int keyCode) {
        return InputSystem::GetKey(keyCode);
    }
    
    static bool GetKeyDown(int keyCode) {
        return InputSystem::GetKeyDown(keyCode);
    }
    
    static bool GetKeyUp(int keyCode) {
        return InputSystem::GetKeyUp(keyCode);
    }
    
    // Mouse input functions
    static bool GetMouseButton(int button) {
        return InputSystem::GetMouseButton(button);
    }
    
    static bool GetMouseButtonDown(int button) {
        return InputSystem::GetMouseButtonDown(button);
    }
    
    static bool GetMouseButtonUp(int button) {
        return InputSystem::GetMouseButtonUp(button);
    }
    
    static glm::vec2 GetMousePosition() {
        double x, y;
        InputSystem::GetMousePosition(x, y);
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }
    
    static glm::vec2 GetMouseDelta() {
        double dx, dy;
        InputSystem::GetMouseDelta(dx, dy);
        return glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
    }
    
    // Convenience functions with key names
    static bool GetKey_W() { return GetKey(KeyCode::W); }
    static bool GetKey_A() { return GetKey(KeyCode::A); }
    static bool GetKey_S() { return GetKey(KeyCode::S); }
    static bool GetKey_D() { return GetKey(KeyCode::D); }
    static bool GetKey_Q() { return GetKey(KeyCode::Q); }
    static bool GetKey_E() { return GetKey(KeyCode::E); }
    static bool GetKey_Space() { return GetKey(KeyCode::Space); }
    static bool GetKey_Enter() { return GetKey(KeyCode::Enter); }
    static bool GetKey_Escape() { return GetKey(KeyCode::Escape); }
    
    static bool GetKeyDown_W() { return GetKeyDown(KeyCode::W); }
    static bool GetKeyDown_A() { return GetKeyDown(KeyCode::A); }
    static bool GetKeyDown_S() { return GetKeyDown(KeyCode::S); }
    static bool GetKeyDown_D() { return GetKeyDown(KeyCode::D); }
    static bool GetKeyDown_Q() { return GetKeyDown(KeyCode::Q); }
    static bool GetKeyDown_E() { return GetKeyDown(KeyCode::E); }
    static bool GetKeyDown_Space() { return GetKeyDown(KeyCode::Space); }
    static bool GetKeyDown_Enter() { return GetKeyDown(KeyCode::Enter); }
    static bool GetKeyDown_Escape() { return GetKeyDown(KeyCode::Escape); }
}; 