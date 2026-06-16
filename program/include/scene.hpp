#pragma once

#include <type_traits>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "core.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class scene
  {
    friend class game;

  protected:
    struct initial_state
    {
    };
    struct initial_graphics
    {
    };

  public:
    virtual ~scene() = default;
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <trait::is_camera camera_type, typename... camera_arguments> scene &set(camera_arguments &&...arguments);
    template <trait::is_object object_type, typename... object_arguments>
    scene &set(const cse::name object_name, object_arguments &&...arguments);
    template <trait::is_interface interface_type, typename... interface_arguments>
    scene &set(const cse::name interface_name, interface_arguments &&...arguments);
    template <typename target_type = void>
      requires(std::is_void_v<target_type> || trait::is_object<target_type> || trait::is_interface<target_type>)
    scene &remove(const cse::name target_name);

  protected:
    scene() = default;
    virtual void pre_prepare();
    virtual void post_prepare();
    virtual void pre_create();
    virtual void post_create();
    virtual void pre_previous();
    virtual void post_previous();
    virtual void pre_sync();
    virtual void post_sync();
    virtual void pre_event(const SDL_Event &event);
    virtual void post_event(const SDL_Event &event);
    virtual void pre_input(const cse::keyboard &keyboard, const cse::mouse &mouse);
    virtual void post_input(const cse::keyboard &keyboard, const cse::mouse &mouse);
    virtual void pre_simulate(const double tick);
    virtual void post_simulate(const double tick);
    virtual void pre_collide(const double tick);
    virtual void post_collide(const double tick);
    virtual void pre_render(const double alpha);
    virtual void post_render(const double alpha);
    virtual void pre_destroy();
    virtual void post_destroy();
    virtual void pre_clean();
    virtual void post_clean();

  private:
    void prepare();
    void create();
    void previous();
    void sync();
    void event(const SDL_Event &event);
    void input(const cse::keyboard &keyboard, const cse::mouse &mouse);
    void simulate(const double tick);
    void collide(const double tick);
    void render(SDL_Window *instance, SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer,
                SDL_GPURenderPass *render_pass, const double previous_aspect, const double active_aspect,
                const double alpha);
    void destroy();
    void clean();

  public:
    cse::game *game{};
    cse::name name{};
    help::scene_state state{};
    help::scene_graphics graphics{};
  };
}

#include "scene.inl" // IWYU pragma: keep
