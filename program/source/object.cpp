#include "object.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"

#include "exception.hpp"
#include "graphics.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse
{
  object::object(const initial_state &state_, const initial_graphics &graphics_)
    : state{state_.translation, state_.rotation, state_.scale, state_.collidable, state_.priority},
      graphics{graphics_.shader, graphics_.texture, graphics_.render, graphics_.priority}
  {
  }

  void object::on_prepare() {}
  void object::prepare()
  {
    if (state.active.phase != help::phase::CLEANED)
      throw exception("Object '{}' must be cleaned before preparation", name.string());
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void object::on_create() {}
  void object::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before creation", name.string());
    graphics.create_pipeline_and_buffers(name, instance, gpu);
    graphics.upload_static_buffers(name, gpu);
    graphics.upload_dynamic_buffers(name, gpu, 1.0);
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void object::on_previous() {}
  void object::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before updating previous state", name.string());
    state.update_previous();
    graphics.update_previous();
    on_previous();
  }

  void object::on_event(const SDL_Event &) {}
  void object::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before processing events", name.string());
    on_event(event);
  }

  void object::on_input(const bool *) {}
  void object::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before processing input", name.string());
    on_input(input);
  }

  void object::on_simulate(const double) {}
  void object::simulate(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    state.active.timer.update(tick);
    graphics.animate(tick);
    on_simulate(tick);
  }

  void object::on_collide(const double) {}
  void object::collide(const double tick)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before simulation", name.string());
    on_collide(tick);
  }

  void object::on_render(const double) {}
  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::dmat4 &projection_matrix, const glm::dmat4 &view_matrix, const double alpha)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before rendering", name.string());
    graphics.upload_dynamic_buffers(name, gpu, alpha);
    graphics.bind_pipeline_and_buffers(render_pass, alpha);
    graphics.push_uniform_data(command_buffer,
                               {projection_matrix, view_matrix,
                                state.calculate_model_matrix(graphics.active.texture.image->frame_width,
                                                             graphics.active.texture.image->frame_height, alpha)},
                               alpha);
    graphics.draw_primitives(render_pass);
    on_render(alpha);
  }

  void object::on_destroy() {}
  void object::destroy(SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object '{}' must be created before destruction", name.string());
    graphics.destroy_resources(gpu);
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void object::on_clean() {}
  void object::clean()
  {
    if (state.active.phase != help::phase::PREPARED)
      throw exception("Object '{}' must be prepared before cleaning", name.string());
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
