#include "object.hpp"

#include <tuple>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "exception.hpp"
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

  void object::prepare()
  {
    if (state.prepared) throw exception("Object cannot be prepared more than once");
    if (state.created) throw exception("Object cannot be prepared while created");
    state.prepared = true;
    hook.call<void()>("prepare");
  }

  void object::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (!state.prepared) throw exception("Object must be prepared before creation");
    if (state.created) throw exception("Object cannot be created more than once");
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu, 1.0);
    state.created = true;
    hook.call<void()>("create");
  }

  void object::previous()
  {
    if (!state.prepared) throw exception("Object must be prepared before updating previous state");
    if (!state.created) throw exception("Object must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
  }

  void object::event(const SDL_Event &event)
  {
    if (!state.prepared) throw exception("Object must be prepared before processing events");
    if (!state.created) throw exception("Object must be created before processing events");
    hook.call<void(const SDL_Event &)>("event", event);
  }

  void object::input(const bool *input)
  {
    if (!state.prepared) throw exception("Object must be prepared before processing input");
    if (!state.created) throw exception("Object must be created before processing input");
    hook.call<void(const bool *)>("input", input);
  }

  void object::simulate(const float poll_rate)
  {
    if (!state.prepared) throw exception("Object must be prepared before simulation");
    if (!state.created) throw exception("Object must be created before simulation");
    graphics.update_animation(poll_rate);
    hook.call<void(const float)>("simulate", poll_rate);
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const double alpha)
  {
    if (!state.prepared) throw exception("Object must be prepared before rendering");
    if (!state.created) throw exception("Object must be created before rendering");
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

  void object::destroy(SDL_GPUDevice *gpu)
  {
    if (!state.prepared) throw exception("Object must be prepared before destruction");
    if (!state.created) throw exception("Object cannot be destroyed more than once");
    graphics.destroy_resources(gpu);
    state.created = false;
    hook.call<void()>("destroy");
  }

  void object::clean()
  {
    if (!state.prepared) throw exception("Object cannot be cleaned more than once");
    if (state.created) throw exception("Object must be destroyed before cleaning");
    state.prepared = false;
    hook.call<void()>("clean");
  }
}
