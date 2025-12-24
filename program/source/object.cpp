#include "object.hpp"

#include <string>
#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "resource.hpp"

namespace cse
{
  object::object(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_, const glm::u8vec4 &tint_,
                 const std::pair<compiled_shader, compiled_shader> &shader_,
                 const std::pair<compiled_texture, std::string> &texture_)
    : state(transform_), graphics(tint_, shader_, texture_)
  {
  }

  object::~object() { hooks.clear(); }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu);
  }

  void object::event(const SDL_Event &event) { hooks.call<void(const SDL_Event &)>("event_main", event); }

  void object::input(const bool *keys) { hooks.call<void(const bool *)>("input_main", keys); }

  void object::simulate(double simulation_alpha)
  {
    state.translation.update();
    state.rotation.update();
    state.scale.update();
    hooks.call<void()>("simulate_main");
    state.translation.interpolate(simulation_alpha);
    state.rotation.interpolate(simulation_alpha);
    state.scale.interpolate(simulation_alpha);
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const float global_scale_factor)
  {
    graphics.upload_dynamic_buffers(gpu);
    graphics.bind_pipeline_and_buffers(render_pass);
    graphics.push_uniform_data(command_buffer, projection_matrix, view_matrix,
                               state.calculate_model_matrix(graphics.texture.data.frame_data.width,
                                                            graphics.texture.data.frame_data.height,
                                                            global_scale_factor));
    graphics.draw_primitives(render_pass);
  }

  void object::cleanup(SDL_GPUDevice *gpu) { graphics.cleanup_object(gpu); }
}
