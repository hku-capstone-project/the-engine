#pragma once

struct ConfigContainer;

class VulkanApplicationContext;
class Logger;
class Window;

class GameStatsGui {
  public:
    GameStatsGui(Logger *logger, ConfigContainer *configContainer, Window *window);
    void update(VulkanApplicationContext *appContext);

  private:
    Logger *_logger;
    ConfigContainer *_configContainer;
    Window *_window;
}; 