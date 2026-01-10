#pragma once

#include <memory>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "name.hpp"
#include "state.hpp"
#include "traits.hpp"
#include "wrapper.hpp"

namespace cse
{
  class scene : public std::enable_shared_from_this<scene>
  {
    friend class game;

  protected:
    struct hook : public enumeration<hook>
    {
      static inline const enumeration_value<hook> PRE_PREPARE{};
      static inline const enumeration_value<hook> POST_PREPARE{};
      static inline const enumeration_value<hook> PRE_CREATE{};
      static inline const enumeration_value<hook> POST_CREATE{};
      static inline const enumeration_value<hook> PRE_SYNC{};
      static inline const enumeration_value<hook> POST_SYNC{};
      static inline const enumeration_value<hook> PRE_EVENT{};
      static inline const enumeration_value<hook> POST_EVENT{};
      static inline const enumeration_value<hook> PRE_INPUT{};
      static inline const enumeration_value<hook> POST_INPUT{};
      static inline const enumeration_value<hook> PRE_SIMULATE{};
      static inline const enumeration_value<hook> POST_SIMULATE{};
      static inline const enumeration_value<hook> PRE_RENDER{};
      static inline const enumeration_value<hook> POST_RENDER{};
      static inline const enumeration_value<hook> PRE_DESTROY{};
      static inline const enumeration_value<hook> POST_DESTROY{};
      static inline const enumeration_value<hook> PRE_CLEAN{};
      static inline const enumeration_value<hook> POST_CLEAN{};
    };

  public:
    scene() = default;
    virtual ~scene();
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <help::is_camera camera_type, typename... camera_arguments>
    std::shared_ptr<scene> set_camera(camera_arguments &&...arguments);
    template <help::is_object object_type, typename... object_arguments>
    std::shared_ptr<scene> set_object(const help::name name, object_arguments &&...arguments);
    std::shared_ptr<scene> remove_object(const help::name name);

  private:
    void prepare();
    void create(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void sync(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const float poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const double alpha, const float aspect_ratio);
    void destroy(SDL_GPUDevice *gpu);
    void clean();

  public:
    help::scene_state state{};
    help::scene_graphics graphics{};
    help::hooks hooks{};
  };
}

#include "scene.inl" // IWYU pragma: keep
