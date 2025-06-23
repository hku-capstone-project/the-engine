#pragma once

#include <unordered_map>
#include <GLFW/glfw3.h>
#include "../window/KeyboardInfo.hpp"
#include "../window/CursorInfo.hpp"

/**
 * Unity-like Input system that provides a unified interface for input queries
 */
class InputSystem {
public:
    // Key state query functions (similar to Unity Input.GetKey/GetKeyDown/GetKeyUp)
    static bool GetKey(int keyCode);
    static bool GetKeyDown(int keyCode);
    static bool GetKeyUp(int keyCode);
    
    // Mouse functions
    static bool GetMouseButton(int button);  // 0=left, 1=right, 2=middle
    static bool GetMouseButtonDown(int button);
    static bool GetMouseButtonUp(int button);
    static void GetMousePosition(double& x, double& y);
    static void GetMouseDelta(double& dx, double& dy);
    
    // System functions - called by the engine, not by scripts
    static void Initialize();
    static void UpdateFrame(const KeyboardInfo& keyboardInfo, const CursorInfo& cursorInfo);
    static void Shutdown();

private:
    // Current frame input state
    static std::unordered_map<int, bool> _currentKeys;
    static std::unordered_map<int, bool> _currentMouseButtons;
    static double _mouseX, _mouseY;
    static double _mouseDX, _mouseDY;
    
    // Previous frame input state (for detecting down/up events)
    static std::unordered_map<int, bool> _previousKeys;
    static std::unordered_map<int, bool> _previousMouseButtons;
    
    static bool _initialized;
};

// Key code constants for convenience (similar to Unity KeyCode enum)
namespace KeyCode {
    const int Space = GLFW_KEY_SPACE;
    const int Enter = GLFW_KEY_ENTER;
    const int Escape = GLFW_KEY_ESCAPE;
    
    // Letters
    const int A = GLFW_KEY_A;
    const int B = GLFW_KEY_B;
    const int C = GLFW_KEY_C;
    const int D = GLFW_KEY_D;
    const int E = GLFW_KEY_E;
    const int F = GLFW_KEY_F;
    const int G = GLFW_KEY_G;
    const int H = GLFW_KEY_H;
    const int I = GLFW_KEY_I;
    const int J = GLFW_KEY_J;
    const int K = GLFW_KEY_K;
    const int L = GLFW_KEY_L;
    const int M = GLFW_KEY_M;
    const int N = GLFW_KEY_N;
    const int O = GLFW_KEY_O;
    const int P = GLFW_KEY_P;
    const int Q = GLFW_KEY_Q;
    const int R = GLFW_KEY_R;
    const int S = GLFW_KEY_S;
    const int T = GLFW_KEY_T;
    const int U = GLFW_KEY_U;
    const int V = GLFW_KEY_V;
    const int W = GLFW_KEY_W;
    const int X = GLFW_KEY_X;
    const int Y = GLFW_KEY_Y;
    const int Z = GLFW_KEY_Z;
    
    // Numbers
    const int Alpha0 = GLFW_KEY_0;
    const int Alpha1 = GLFW_KEY_1;
    const int Alpha2 = GLFW_KEY_2;
    const int Alpha3 = GLFW_KEY_3;
    const int Alpha4 = GLFW_KEY_4;
    const int Alpha5 = GLFW_KEY_5;
    const int Alpha6 = GLFW_KEY_6;
    const int Alpha7 = GLFW_KEY_7;
    const int Alpha8 = GLFW_KEY_8;
    const int Alpha9 = GLFW_KEY_9;
    
    // Arrows
    const int UpArrow = GLFW_KEY_UP;
    const int DownArrow = GLFW_KEY_DOWN;
    const int LeftArrow = GLFW_KEY_LEFT;
    const int RightArrow = GLFW_KEY_RIGHT;
    
    // Modifiers
    const int LeftShift = GLFW_KEY_LEFT_SHIFT;
    const int RightShift = GLFW_KEY_RIGHT_SHIFT;
    const int LeftControl = GLFW_KEY_LEFT_CONTROL;
    const int RightControl = GLFW_KEY_RIGHT_CONTROL;
    const int LeftAlt = GLFW_KEY_LEFT_ALT;
    const int RightAlt = GLFW_KEY_RIGHT_ALT;
}

// Mouse button constants
namespace MouseButton {
    const int Left = 0;
    const int Right = 1;
    const int Middle = 2;
} 