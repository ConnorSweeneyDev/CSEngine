#pragma once

#include <memory>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "name.hpp"
#include "state.hpp"
#include "traits.hpp"

namespace cse
{
  class scene : public std::enable_shared_from_this<scene>
  {
    friend class game;

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
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void update();
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const float poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const double alpha, const float aspect_ratio);
    void cleanup(SDL_GPUDevice *gpu);

  public:
    help::scene_state state{};
    help::scene_graphics graphics{};
    help::hook hook{};
  };
}

#include "scene.inl" // IWYU pragma: keep
