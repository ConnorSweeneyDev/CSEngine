#include "object.hpp"

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
                 const std::pair<shader, shader> &shader_, const std::pair<image, group> &texture_)
    : state(transform_), graphics(tint_, shader_, texture_)
  {
  }

  object::~object()
  {
    hooks.clear();
    parent.reset();
  }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu);
    initialized = true;
    hooks.call<void()>("initialize");
  }

  void object::event(const SDL_Event &event) { hooks.call<void(const SDL_Event &)>("event", event); }

  void object::input(const bool *keys) { hooks.call<void(const bool *)>("input", keys); }

  void object::simulate(const double time)
  {
    state.translation.update();
    state.rotation.update();
    state.scale.update();

    auto &animation = graphics.texture.animation;
    auto &group = graphics.texture.group;
    auto &previous = graphics.texture.previous;
    previous.group = group;
    previous.frame = animation.frame;
    previous.elapsed = animation.elapsed;
    if (animation.speed > 0.0 && !group.frames.empty() && animation.frame < group.frames.size())
    {
      static double frame_time{time};
      animation.elapsed += (time - frame_time) * animation.speed;
      frame_time = time;
      auto duration = group.frames[animation.frame].duration;
      if (animation.elapsed >= duration)
      {
        if (animation.frame < group.frames.size() - 1)
        {
          animation.elapsed -= duration;
          animation.frame++;
        }
        else
        {
          animation.elapsed = duration;
          animation.speed = 0.0;
        }
      }
    }

    hooks.call<void()>("simulate");
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const double alpha,
                      const float scale_factor)
  {
    state.translation.interpolate(alpha);
    state.rotation.interpolate(alpha);
    state.scale.interpolate(alpha);
    graphics.upload_dynamic_buffers(gpu);
    graphics.bind_pipeline_and_buffers(render_pass);
    auto model_matrix{state.calculate_model_matrix(graphics.texture.image.frame_width,
                                                   graphics.texture.image.frame_height, scale_factor)};
    graphics.push_uniform_data(command_buffer, projection_matrix, view_matrix, model_matrix);
    graphics.draw_primitives(render_pass);
    hooks.call<void(const glm::mat4 &)>("render", model_matrix);
  }

  void object::cleanup(SDL_GPUDevice *gpu)
  {
    graphics.cleanup_object(gpu);
    initialized = false;
    hooks.call<void()>("cleanup");
  }
}
