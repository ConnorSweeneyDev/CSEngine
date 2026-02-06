#include "object.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "exception.hpp"
#include "graphics.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse
{
  object::object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_,
                 const std::pair<vertex, fragment> &shader_,
                 const std::tuple<image, group, animation, flip, glm::vec4, double> &texture_,
                 const std::tuple<int> &property_)
    : state{transform_}, graphics{shader_, texture_, property_}
  {
  }

  void object::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Object must be cleaned before preparation");
    state.active.phase = help::phase::PREPARED;
    hooks.call<void()>(hook::PREPARE);
  }

  void object::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Object must be prepared before creation");
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu, 1.0);
    state.active.phase = help::phase::CREATED;
    hooks.call<void()>(hook::CREATE);
  }

  void object::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
  }

  void object::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before processing events");
    hooks.call<void(const SDL_Event &)>(hook::EVENT, event);
  }

  void object::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before processing input");
    hooks.call<void(const bool *)>(hook::INPUT, input);
  }

  void object::simulate(const float poll_rate)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before simulation");
    timers.update(poll_rate);
    graphics.update_animation(poll_rate);
    hooks.call<void(const float)>(hook::SIMULATE, poll_rate);
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::dmat4 &projection_matrix, const glm::dmat4 &view_matrix, const double alpha)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before rendering");
    graphics.upload_dynamic_buffers(gpu, alpha);
    graphics.bind_pipeline_and_buffers(render_pass, alpha);
    graphics.push_uniform_data(command_buffer,
                               {projection_matrix, view_matrix,
                                state.calculate_model_matrix(graphics.active.texture.image->frame_width,
                                                             graphics.active.texture.image->frame_height, alpha)},
                               alpha);
    graphics.draw_primitives(render_pass);
    hooks.call<void(const double)>(hook::RENDER, alpha);
  }

  void object::destroy(SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before destruction");
    graphics.destroy_resources(gpu);
    state.active.phase = help::phase::PREPARED;
    hooks.call<void()>(hook::DESTROY);
  }

  void object::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Object must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    hooks.call<void()>(hook::CLEAN);
  }
}
