#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "declaration.hpp"
#include "object.hpp"

namespace cse::core
{
  class scene
  {
    friend class game;

  public:
    scene();
    virtual ~scene();
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <typename camera_type, typename... camera_arguments> void set_camera(camera_arguments &&...arguments);
    template <typename object_type, typename... object_arguments>
    void add_object(const std::string &name, object_arguments &&...arguments);

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void cleanup(SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double simulation_alpha);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const float target_aspect_ratio, const float global_scale_factor);

  protected:
    std::function<void(const SDL_Event &key)> handle_event = {};
    std::function<void(const bool *keys)> handle_input = {};
    std::function<void(const double simulation_alpha)> handle_simulate = {};

  private:
    std::unique_ptr<class camera> camera = {};
    std::unordered_map<std::string, std::shared_ptr<object>> objects = {};
  };
}

#include "scene.inl"
