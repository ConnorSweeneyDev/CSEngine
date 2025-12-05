#include "object.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint4_sized.hpp"

#include "resource.hpp"

namespace cse::core
{
  object::object(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_,
                 const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_,
                 const resource::compiled_texture &texture_, const std::string &frame_group_, const glm::u8vec4 &tint_)
    : state({translation_.x, translation_.y, translation_.z}, {rotation_.x, rotation_.y, rotation_.z},
            {scale_.x, scale_.y, scale_.z}),
      graphics(vertex_shader_, fragment_shader_, texture_, frame_group_, tint_)
  {
  }

  object::~object()
  {
    handle_simulate = nullptr;
    handle_input = nullptr;
    handle_event = nullptr;
  }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu);
  }

  void object::cleanup(SDL_GPUDevice *gpu) { graphics.cleanup_object(gpu); }

  void object::event(const SDL_Event &event)
  {
    if (handle_event) handle_event(event);
  }

  void object::input(const bool *keys)
  {
    if (handle_input) handle_input(keys);
  }

  void object::simulate(double simulation_alpha)
  {
    state.translation.update();
    state.rotation.update();
    state.scale.update();
    if (handle_simulate) handle_simulate();
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
                                                            graphics.texture.data.frame_data.width,
                                                            global_scale_factor));
    graphics.draw_primitives(render_pass);
  }
}
