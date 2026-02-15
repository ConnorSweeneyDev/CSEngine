#pragma once

#include <memory>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "declaration.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class scene : public std::enable_shared_from_this<scene>
  {
    friend class game;

  public:
    virtual ~scene() = default;
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <trait::is_camera camera_type, typename... camera_arguments> scene &set(camera_arguments &&...arguments);
    template <trait::is_object object_type, typename... object_arguments>
    scene &set(const name name, object_arguments &&...arguments);
    scene &remove(const name name);

  protected:
    scene() = default;
    virtual void pre_prepare() {}
    virtual void post_prepare() {}
    virtual void pre_create() {}
    virtual void post_create() {}
    virtual void pre_previous() {}
    virtual void post_previous() {}
    virtual void pre_sync() {}
    virtual void post_sync() {}
    virtual void pre_event(const SDL_Event &) {}
    virtual void post_event(const SDL_Event &) {}
    virtual void pre_input(const bool *) {}
    virtual void post_input(const bool *) {}
    virtual void pre_simulate(const double) {}
    virtual void post_simulate(const double) {}
    virtual void pre_render(const double) {}
    virtual void post_render(const double) {}
    virtual void pre_destroy() {}
    virtual void post_destroy() {}
    virtual void pre_clean() {}
    virtual void post_clean() {}

  private:
    void prepare();
    void create(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void sync(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const double poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const double alpha, const double previous_aspect_ratio, const double active_aspect_ratio);
    void destroy(SDL_GPUDevice *gpu);
    void clean();

  public:
    help::scene_state state{};
    help::scene_graphics graphics{};
  };
}

#include "scene.inl" // IWYU pragma: keep
