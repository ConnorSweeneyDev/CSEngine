#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "declaration.hpp"
#include "hook.hpp"
#include "id.hpp"
#include "object.hpp"
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
    std::shared_ptr<scene> set_object(const help::id name, object_arguments &&...arguments);
    void remove_object(const help::id name);

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double active_poll_rate);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const double alpha, const float aspect_ratio, const float scale_factor);
    void cleanup(SDL_GPUDevice *gpu);

    void process_updates();
    void update_previous();

  public:
    std::weak_ptr<game> parent{};
    std::shared_ptr<class camera> camera{};
    std::unordered_map<help::id, std::shared_ptr<class object>> objects{};
    help::hook hook{};

  private:
    bool initialized{};
    std::unordered_set<help::id> removals{};
    std::unordered_map<help::id, std::shared_ptr<object>> additions{};
  };
}

#include "scene.inl" // IWYU pragma: keep
