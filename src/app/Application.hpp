#pragma once

#include "window/Window.hpp"

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/task_graph.hpp>

#include <memory>

class Application {
public:
  Application();
  ~Application();

  void run();

private:
  std::unique_ptr<AppWindow> window = nullptr;

  BufferId buffer_id = {};

  std::unique_ptr<daxa::Instance> instance                = nullptr;
  std::unique_ptr<daxa::Device> device                    = nullptr;
  std::unique_ptr<daxa::Swapchain> swapchain              = nullptr;
  std::unique_ptr<daxa::PipelineManager> pipeline_manager = nullptr;
  std::shared_ptr<daxa::RasterPipeline> raster_pipeline   = nullptr;

  std::unique_ptr<daxa::TaskImage> task_swapchain_image = nullptr;
  std::unique_ptr<daxa::TaskGraph> loop_task_graph      = nullptr;
};
