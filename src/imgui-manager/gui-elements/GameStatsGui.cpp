#include "GameStatsGui.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/ImguiManagerInfo.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"
#include "dotnet/RuntimeBridge.hpp"
#include "dotnet/RuntimeApplication.hpp"
#include "dotnet/Components.hpp"

#include "imgui.h"

GameStatsGui::GameStatsGui(Logger *logger, ConfigContainer *configContainer, Window *window)
    : _logger(logger), _configContainer(configContainer), _window(window) {}

void GameStatsGui::update(VulkanApplicationContext *appContext) {
    int windowWidth  = 0;
    int windowHeight = 0;
    _window->getWindowDimension(windowWidth, windowHeight);

    float constexpr kWindowWidth = 200.0f;
    float constexpr kWindowHeight = 100.0f;
    float constexpr kPadding = 10.0f;

    // 在右上角显示
    ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight));
    ImGui::SetNextWindowPos(ImVec2(windowWidth - kWindowWidth - kPadding, kPadding + 30)); // 30是菜单栏高度

    if (!ImGui::Begin("Game Stats", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        _logger->error("Failed to create game stats window!");
        ImGui::End();
        return;
    }

    // 直接从ECS registry查询GameStats组件（符合ECS标准）
    auto& registry = RuntimeBridge::getRuntimeApplication().registry;
    auto gameStatsView = registry.view<GameStats>();
    
    int killCount = 0;
    float gameTime = 0.0f;
    
    // 查找GameStats组件
    for (auto entity : gameStatsView) {
        auto& gameStats = gameStatsView.get<GameStats>(entity);
        killCount = gameStats.killCount;
        gameTime = gameStats.gameTime;
        break; // 只需要第一个GameStats实体
    }

    // 显示击杀数
    ImGui::Text("Kills: %d", killCount);
    
    // 显示游戏时间（格式化为 分:秒）
    int minutes = (int)(gameTime / 60.0f);
    int seconds = (int)(gameTime) % 60;
    ImGui::Text("Time: %02d:%02d", minutes, seconds);

    ImGui::End();
} 