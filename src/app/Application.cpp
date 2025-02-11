#include "Application.hpp"
#include "window/Window.hpp"
#include "config/RootDir.h"

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/task_graph.hpp>

#include <memory>

#include "shader/shared.inl"


void upload_vertex_data_task(daxa::TaskGraph &tg, daxa::TaskBufferView vertices) {
  tg.add_task({
      .attachments =
          {
              daxa::inl_attachment(daxa::TaskBufferAccess::TRANSFER_WRITE, vertices),
          },
      .task =
          [=](daxa::TaskInterface ti) {
            auto data = std::array{
                MyVertex{.position = {-0.5f, +0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}},
                MyVertex{.position = {+0.5f, +0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}},
                MyVertex{.position = {+0.0f, -0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f}},
            };
            auto staging_buffer_id = ti.device.create_buffer({
                .size          = sizeof(data),
                .allocate_info = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                .name          = "my staging buffer",
            });
            ti.recorder.destroy_buffer_deferred(staging_buffer_id);

            auto *buffer_ptr =
                ti.device.buffer_host_address_as<std::array<MyVertex, 3>>(staging_buffer_id)
                    .value();
            *buffer_ptr = data;
            ti.recorder.copy_buffer_to_buffer({
                .src_buffer = staging_buffer_id,
                .dst_buffer = ti.get(vertices).ids[0],
                .size       = sizeof(data),
            });
          },
  });
}

void draw_vertices_task(daxa::TaskGraph &tg, std::shared_ptr<daxa::RasterPipeline> pipeline,
                        daxa::TaskBufferView vertices, daxa::TaskImageView render_target) {
  tg.add_task({
      .attachments =
          {
              daxa::inl_attachment(daxa::TaskBufferAccess::VERTEX_SHADER_READ, vertices),
              daxa::inl_attachment(daxa::TaskImageAccess::COLOR_ATTACHMENT,
                                   daxa::ImageViewType::REGULAR_2D, render_target),
          },
      .task =
          [=](daxa::TaskInterface ti) {
            auto const size = ti.device.image_info(ti.get(render_target).ids[0]).value().size;

            daxa::RenderCommandRecorder render_recorder =
                std::move(ti.recorder)
                    .begin_renderpass({
                        .color_attachments =
                            std::array{
                                daxa::RenderAttachmentInfo{
                                    .image_view  = ti.get(render_target).view_ids[0],
                                    .load_op     = daxa::AttachmentLoadOp::CLEAR,
                                    .clear_value = std::array<daxa::f32, 4>{0.1f, 0.0f, 0.5f, 1.0f},
                                },
                            },
                        .render_area = {.width = size.x, .height = size.y},
                    });

            render_recorder.set_pipeline(*pipeline);
            render_recorder.push_constant(MyPushConstant{
                .my_vertex_ptr = ti.device.buffer_device_address(ti.get(vertices).ids[0]).value(),
            });
            render_recorder.draw({.vertex_count = 3});
            ti.recorder = std::move(render_recorder).end_renderpass();
          },
      .name = "draw vertices",
  });
}

Application::Application() {
  window = std::make_unique<AppWindow>("Learn Daxa", 860, 640);

  instance = std::make_unique<daxa::Instance>(daxa::create_instance({}));

  device =
      std::make_unique<daxa::Device>(instance->create_device_2(instance->choose_device({}, {})));

  swapchain = std::make_unique<daxa::Swapchain>(device->create_swapchain({
      // this handle is given by the windowing API
      .native_window = window->get_native_handle(),
      // The platform would also be retrieved from the windowing API,
      // or by hard-coding it depending on the OS.
      .native_window_platform = window->get_native_platform(),
      // Here we can supply a user-defined surface format selection
      // function, to rate formats. If you don't care what format the
      // swapchain images are in, then you can just omit this argument
      // because it defaults to `daxa::default_format_score(...)`
      .surface_format_selector =
          [](daxa::Format format) {
            switch (format) {
            case daxa::Format::R8G8B8A8_UINT:
              return 100;
            default:
              return daxa::default_format_score(format);
            }
          },
      .present_mode = daxa::PresentMode::MAILBOX,
      .image_usage  = daxa::ImageUsageFlagBits::TRANSFER_DST,
      .name         = "my swapchain",
  }));

  pipeline_manager = std::make_unique<daxa::PipelineManager>(daxa::PipelineManager({
      .device = *device,
      .shader_compile_options =
          {
              .root_paths =
                  {
                      DAXA_SHADER_INCLUDE_DIR,
                      ROOT_DIR + "src/shader/",
                  },
              .language          = daxa::ShaderLanguage::GLSL,
              .enable_debug_info = true,
          },
      .name = "my pipeline manager",
  }));

  // create raster pipeline
  auto result = pipeline_manager->add_raster_pipeline({
      .vertex_shader_info   = daxa::ShaderCompileInfo{.source = daxa::ShaderFile{"main.glsl"}},
      .fragment_shader_info = daxa::ShaderCompileInfo{.source = daxa::ShaderFile{"main.glsl"}},
      .color_attachments    = {{.format = swapchain->get_format()}},
      .raster               = {},
      .push_constant_size   = sizeof(MyPushConstant),
      .name                 = "my pipeline",
  });
  if (result.is_err()) {
    std::cerr << result.message() << std::endl;
  }
  raster_pipeline = result.value();

  buffer_id               = device->create_buffer({
                    .size = sizeof(MyVertex) * 3,
                    .name = "my vertex data",
  });
  auto task_vertex_buffer = daxa::TaskBuffer({
      .initial_buffers = {.buffers = std::span{&buffer_id, 1}},
      .name            = "task vertex buffer",
  });

  {
    auto upload_task_graph = daxa::TaskGraph({
        .device = *device,
        .name   = "upload",
    });

    upload_task_graph.use_persistent_buffer(task_vertex_buffer);

    upload_vertex_data_task(upload_task_graph, task_vertex_buffer);

    upload_task_graph.submit({});
    upload_task_graph.complete({});
    upload_task_graph.execute({});
  }

  loop_task_graph = std::make_unique<daxa::TaskGraph>(daxa::TaskGraph({
      .device    = *device,
      .swapchain = *swapchain,
      .name      = "loop",
  }));

  task_swapchain_image = std::make_unique<daxa::TaskImage>(
      daxa::TaskImage{{.swapchain_image = true, .name = "swapchain image"}});

  loop_task_graph->use_persistent_buffer(task_vertex_buffer);
  loop_task_graph->use_persistent_image(*task_swapchain_image);

  draw_vertices_task(*loop_task_graph, raster_pipeline, task_vertex_buffer, *task_swapchain_image);

  loop_task_graph->submit({});
  // And tell the task graph to do the present step.
  loop_task_graph->present({});
  // Finally, we complete the task graph, which essentially compiles the
  // dependency graph between tasks, and inserts the most optimal synchronization!
  loop_task_graph->complete({});
}

Application::~Application() {
  device->destroy_buffer(buffer_id);
  device->wait_idle();
  device->collect_garbage();
}

void Application::run() {
  // Daxa rendering initialization code goes here...

  while (!window->should_close()) {
    window->update();

    if (window->is_swapchain_out_of_date()) {
      swapchain->resize();
      window->set_swapchain_out_of_date(false);
    }

    // acquire the next image
    auto swapchain_image = swapchain->acquire_next_image();
    if (swapchain_image.is_empty()) {
      continue;
    }

    // We update the image id of the task swapchain image.
    task_swapchain_image->set_images({.images = std::span{&swapchain_image, 1}});

    // So, now all we need to do is execute our task graph!
    loop_task_graph->execute({});
    device->collect_garbage();
  }
}
