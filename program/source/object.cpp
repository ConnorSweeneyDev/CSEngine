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
    : state{transform_}, graphics{shader_, texture_, property_}, previous{state, graphics}
  {
  }

  object::~object()
  {
    hook.reset();
    parent.reset();
  }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline_and_buffers(instance, gpu);
    graphics.upload_static_buffers(gpu);
    graphics.upload_dynamic_buffers(gpu);
    initialized = true;
    hook.call<void()>("initialize");
  }

  void object::event(const SDL_Event &event) { hook.call<void(const SDL_Event &)>("event", event); }

  void object::input(const bool *keys) { hook.call<void(const bool *)>("input", keys); }

  void object::simulate(const double active_poll_rate)
  {
    state.translation.update();
    state.rotation.update();
    state.scale.update();
    auto &group{graphics.texture.group};
    auto &animation{graphics.texture.animation};
    auto frame_count{group.frames.size()};
    bool no_frames{group.frames.empty()};
    if (no_frames)
      animation.frame = 0;
    else if (animation.frame >= frame_count)
      animation.frame = frame_count - 1;
    if (animation.speed > 0.0 && !no_frames)
    {
      animation.elapsed += active_poll_rate * animation.speed;
      while (true)
      {
        auto duration = group.frames[animation.frame].duration;
        if (duration > 0 && animation.elapsed < duration) break;
        if (animation.frame < frame_count - 1)
        {
          if (duration > 0) animation.elapsed -= duration;
          animation.frame++;
        }
        else if (animation.loop)
        {
          if (duration > 0)
            animation.elapsed -= duration;
          else
            break;
          animation.frame = 0;
        }
        else
          break;
      }
    }
    else if (animation.speed < 0.0 && !no_frames)
    {
      animation.elapsed += active_poll_rate * animation.speed;
      while (animation.elapsed < 0)
        if (animation.frame > 0)
        {
          animation.frame--;
          auto duration = group.frames[animation.frame].duration;
          if (duration > 0) animation.elapsed += duration;
        }
        else if (animation.loop)
        {
          if (group.frames[0].duration <= 0) break;
          animation.frame = frame_count - 1;
          auto duration = group.frames[animation.frame].duration;
          if (duration > 0) animation.elapsed += duration;
        }
        else
          break;
    }
    hook.call<void(const float)>("simulate", static_cast<float>(active_poll_rate));
  }

  void object::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, const float scale_factor)
  {
    graphics.upload_dynamic_buffers(gpu);
    graphics.bind_pipeline_and_buffers(render_pass);
    auto model_matrix{state.calculate_model_matrix(graphics.texture.image->frame_width,
                                                   graphics.texture.image->frame_height, scale_factor)};
    graphics.push_uniform_data(command_buffer, {projection_matrix, view_matrix, model_matrix});
    graphics.draw_primitives(render_pass);
    hook.call<void(const glm::mat4 &)>("render", model_matrix);
  }

  void object::cleanup(SDL_GPUDevice *gpu)
  {
    graphics.cleanup_object(gpu);
    initialized = false;
    hook.call<void()>("cleanup");
  }
}
