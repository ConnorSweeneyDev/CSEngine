#include "object.hpp"

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "resource.hpp"

namespace cse::core
{
  object::object(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_,
                 const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_,
                 const resource::compiled_texture &texture_, const std::string &current_group_)
    : transform({translation_.x, translation_.y, translation_.z}, {rotation_.x, rotation_.y, rotation_.z},
                {scale_.x, scale_.y, scale_.z}),
      graphics(vertex_shader_, fragment_shader_, texture_, current_group_)
  {
  }

  object::~object()
  {
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline(instance, gpu);
    graphics.create_vertex_and_index(gpu);
    graphics.create_sampler_and_texture(gpu);
    graphics.transfer_vertex_and_index(gpu);
    graphics.transfer_texture(gpu);
    graphics.upload_to_gpu(gpu);
  }

  void object::cleanup(SDL_GPUDevice *gpu) { graphics.cleanup_object(gpu); }

  void object::event(const SDL_Event &event)
  {
    switch (event.type)
    {
      case SDL_EVENT_KEY_DOWN:
        if (handle_event) handle_event(event.key);
        break;
      default: break;
    }
  }

  void object::input(const bool *keys)
  {
    if (handle_input) handle_input(keys);
  }

  void object::simulate(double simulation_alpha)
  {
    transform.translation.previous = transform.translation.value;
    transform.rotation.previous = transform.rotation.value;
    transform.scale.previous = transform.scale.value;

    if (handle_simulate) handle_simulate();

    transform.translation.interpolate(simulation_alpha);
    transform.rotation.interpolate(simulation_alpha);
    transform.scale.interpolate(simulation_alpha);
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const float scale_factor)
  {
    graphics.update_vertex(gpu);
    graphics.bind_pipeline_and_buffers(render_pass);
    graphics.push_uniform_data(command_buffer,
                               graphics.calculate_model_matrix(transform.translation.interpolated,
                                                               transform.rotation.interpolated,
                                                               transform.scale.interpolated, scale_factor),
                               projection_matrix, view_matrix);
    graphics.draw_primitives(render_pass);
  }
}
