#include "object.hpp"

#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_int3.hpp"

#include "collision.hpp"
#include "exception.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse
{
  object::object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_,
                 const std::pair<vertex, fragment> &shader_,
                 const std::tuple<image, animation, playback, flip, color, transparency> &texture_,
                 const std::tuple<int> &property_)
    : state{transform_}, graphics{shader_, texture_, property_}
  {
  }

  void object::prepare()
  {
    if (state.active.phase != help::phase::CLEANED) throw exception("Object must be cleaned before preparation");
    state.active.phase = help::phase::PREPARED;
    on_prepare();
  }

  void object::create(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Object must be prepared before creation");
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu, 1.0);
    state.active.phase = help::phase::CREATED;
    on_create();
  }

  void object::previous()
  {
    if (state.active.phase != help::phase::CREATED)
      throw exception("Object must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
    on_previous();
  }

  void object::event(const SDL_Event &event)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before processing events");
    on_event(event);
  }

  void object::input(const bool *input)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before processing input");
    on_input(input);
  }

  void object::simulate(const double poll_rate)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before simulation");
    state.active.timer.update(poll_rate);
    graphics.animate(poll_rate);
    on_simulate(poll_rate);
  }

  void object::collide(const double poll_rate, const name self,
                       const std::unordered_map<name, std::shared_ptr<object>> &objects)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before simulation");
    state.active.collision.update(self, objects);
    on_collide(poll_rate);
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
    on_render(alpha);
  }

  void object::destroy(SDL_GPUDevice *gpu)
  {
    if (state.active.phase != help::phase::CREATED) throw exception("Object must be created before destruction");
    graphics.destroy_resources(gpu);
    state.active.phase = help::phase::PREPARED;
    on_destroy();
  }

  void object::clean()
  {
    if (state.active.phase != help::phase::PREPARED) throw exception("Object must be prepared before cleaning");
    state.active.phase = help::phase::CLEANED;
    on_clean();
  }
}
