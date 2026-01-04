#include "object.hpp"

#include <tuple>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "graphics.hpp"
#include "state.hpp"

namespace cse
{
  object::object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_,
                 const struct help::object_graphics::shader &shader_,
                 const struct help::object_graphics::texture &texture_,
                 const struct help::object_graphics::property &property_)
    : state{transform_}, graphics{shader_, texture_, property_}
  {
  }

  object::~object() { hook.reset(); }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu);
    state.initialized = true;
    hook.call<void()>("initialize");
  }

  void object::event(const SDL_Event &event) { hook.call<void(const SDL_Event &)>("event", event); }

  void object::input(const bool *keys) { hook.call<void(const bool *)>("input", keys); }

  void object::simulate(const double active_poll_rate)
  {
    state.active.translation.update_previous();
    state.active.rotation.update_previous();
    state.active.scale.update_previous();
    graphics.update_animation(active_poll_rate);
    hook.call<void(const float)>("simulate", static_cast<float>(active_poll_rate));
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix)
  {
    graphics.upload_dynamic_buffers(gpu);
    graphics.bind_pipeline_and_buffers(render_pass);
    auto model_matrix{state.calculate_model_matrix(graphics.active.texture.image->frame_width,
                                                   graphics.active.texture.image->frame_height)};
    graphics.push_uniform_data(command_buffer, {projection_matrix, view_matrix, model_matrix});
    graphics.draw_primitives(render_pass);
    hook.call<void(const glm::mat4 &)>("render", model_matrix);
  }

  void object::cleanup(SDL_GPUDevice *gpu)
  {
    graphics.cleanup_object(gpu);
    state.initialized = false;
    hook.call<void()>("cleanup");
  }

  void object::update_previous()
  {
    state.update_previous();
    graphics.update_previous();
  }
}
