#include "InputSystem.hpp"
#include <iostream>

// Static member definitions
std::unordered_map<int, bool> InputSystem::_currentKeys;
std::unordered_map<int, bool> InputSystem::_currentMouseButtons;
double InputSystem::_mouseX = 0.0;
double InputSystem::_mouseY = 0.0;
double InputSystem::_mouseDX = 0.0;
double InputSystem::_mouseDY = 0.0;
std::unordered_map<int, bool> InputSystem::_previousKeys;
std::unordered_map<int, bool> InputSystem::_previousMouseButtons;
bool InputSystem::_initialized = false;

void InputSystem::Initialize() {
    if (_initialized) {
        return;
    }
    
    _currentKeys.clear();
    _currentMouseButtons.clear();
    _previousKeys.clear();
    _previousMouseButtons.clear();
    
    _mouseX = _mouseY = 0.0;
    _mouseDX = _mouseDY = 0.0;
    
    _initialized = true;
    std::cout << "InputSystem initialized\n";
}

void InputSystem::UpdateFrame(const KeyboardInfo& keyboardInfo, const CursorInfo& cursorInfo) {
    if (!_initialized) {
        Initialize();
    }
    
    // Save previous frame state
    _previousKeys = _currentKeys;
    _previousMouseButtons = _currentMouseButtons;
    
    // Update current frame keyboard state
    _currentKeys = keyboardInfo.activeKeyMap;
    
    // Update current frame mouse state
    _currentMouseButtons[MouseButton::Left] = cursorInfo.leftButtonPressed;
    _currentMouseButtons[MouseButton::Right] = cursorInfo.rightButtonPressed;
    _currentMouseButtons[MouseButton::Middle] = cursorInfo.middleButtonPressed;
    
    // Update mouse position and delta
    _mouseX = cursorInfo.cursorMoveInfo.currentX;
    _mouseY = cursorInfo.cursorMoveInfo.currentY;
    _mouseDX = cursorInfo.cursorMoveInfo.dx;
    _mouseDY = cursorInfo.cursorMoveInfo.dy;
}

void InputSystem::Shutdown() {
    _currentKeys.clear();
    _currentMouseButtons.clear();
    _previousKeys.clear();
    _previousMouseButtons.clear();
    _initialized = false;
    std::cout << "InputSystem shutdown\n";
}

bool InputSystem::GetKey(int keyCode) {
    if (!_initialized) return false;
    
    auto it = _currentKeys.find(keyCode);
    return (it != _currentKeys.end()) ? it->second : false;
}

bool InputSystem::GetKeyDown(int keyCode) {
    if (!_initialized) return false;
    
    // Key is pressed this frame but was not pressed last frame
    bool currentlyPressed = GetKey(keyCode);
    auto prevIt = _previousKeys.find(keyCode);
    bool previouslyPressed = (prevIt != _previousKeys.end()) ? prevIt->second : false;
    
    return currentlyPressed && !previouslyPressed;
}

bool InputSystem::GetKeyUp(int keyCode) {
    if (!_initialized) return false;
    
    // Key was pressed last frame but is not pressed this frame
    bool currentlyPressed = GetKey(keyCode);
    auto prevIt = _previousKeys.find(keyCode);
    bool previouslyPressed = (prevIt != _previousKeys.end()) ? prevIt->second : false;
    
    return !currentlyPressed && previouslyPressed;
}

bool InputSystem::GetMouseButton(int button) {
    if (!_initialized) return false;
    
    auto it = _currentMouseButtons.find(button);
    return (it != _currentMouseButtons.end()) ? it->second : false;
}

bool InputSystem::GetMouseButtonDown(int button) {
    if (!_initialized) return false;
    
    bool currentlyPressed = GetMouseButton(button);
    auto prevIt = _previousMouseButtons.find(button);
    bool previouslyPressed = (prevIt != _previousMouseButtons.end()) ? prevIt->second : false;
    
    return currentlyPressed && !previouslyPressed;
}

bool InputSystem::GetMouseButtonUp(int button) {
    if (!_initialized) return false;
    
    bool currentlyPressed = GetMouseButton(button);
    auto prevIt = _previousMouseButtons.find(button);
    bool previouslyPressed = (prevIt != _previousMouseButtons.end()) ? prevIt->second : false;
    
    return !currentlyPressed && previouslyPressed;
}

void InputSystem::GetMousePosition(double& x, double& y) {
    x = _mouseX;
    y = _mouseY;
}

void InputSystem::GetMouseDelta(double& dx, double& dy) {
    dx = _mouseDX;
    dy = _mouseDY;
} 