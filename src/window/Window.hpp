#pragma once

#include <daxa/daxa.hpp>
using namespace daxa::types; // For types like `u32`

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
using HWND = void *;
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3native.h> // Platform-specific GLFW functions

class AppWindow {
public:
  AppWindow(char const *window_name, u32 sx = 800, u32 sy = 600);

  ~AppWindow();

  daxa::NativeWindowHandle get_native_handle() const;

  static daxa::NativeWindowPlatform get_native_platform();

  void set_mouse_capture(bool should_capture) const;

  bool should_close() const;

  void update() const;

  GLFWwindow *get_glfw_window() const;

  bool is_swapchain_out_of_date() const { return swapchain_out_of_date; }

  void set_swapchain_out_of_date(bool value) { swapchain_out_of_date = value; }

  bool is_minimized() const { return minimized; }

  void set_minimized(bool value) { minimized = value; }

private:
  // Window-related properties and methods will go here
  GLFWwindow *glfw_window_ptr;        // Pointer to the GLFW window object
  u32 width, height;                  // Dimensions of the window
  bool minimized             = false; // Tracks if the window is minimized
  bool swapchain_out_of_date = false; // Tracks if the swapchain needs updating
};
