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
    graphics.upload_dynamic_buffers(gpu, 1.0);
    state.initialized = true;
    hook.call<void()>("initialize");
  }

  void object::previous()
  {
    state.update_previous();
    graphics.update_previous();
  }

  void object::event(const SDL_Event &event) { hook.call<void(const SDL_Event &)>("event", event); }

  void object::input(const bool *input) { hook.call<void(const bool *)>("input", input); }

  void object::simulate(const float poll_rate)
  {
    graphics.update_animation(poll_rate);
    hook.call<void(const float)>("simulate", poll_rate);
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const double alpha)
  {
    graphics.upload_dynamic_buffers(gpu, alpha);
    graphics.bind_pipeline_and_buffers(render_pass, alpha);
    graphics.push_uniform_data(command_buffer,
                               {projection_matrix, view_matrix,
                                state.calculate_model_matrix(graphics.active.texture.image->frame_width,
                                                             graphics.active.texture.image->frame_height, alpha)},
                               alpha);
    graphics.draw_primitives(render_pass);
    hook.call<void(const double)>("render", alpha);
  }

  void object::cleanup(SDL_GPUDevice *gpu)
  {
    graphics.cleanup_object(gpu);
    state.initialized = false;
    hook.call<void()>("cleanup");
  }
}
