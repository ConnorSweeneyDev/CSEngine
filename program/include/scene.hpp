#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "declaration.hpp"

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
    void input(const bool *key_state);
    void simulate(const double simulation_alpha);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const unsigned int width, const unsigned int height, const float scale_factor);

  protected:
    std::function<void(const bool *key_state)> handle_input = {};
    std::function<void(const double simulation_alpha)> handle_simulate = {};

  private:
    std::unique_ptr<camera> camera = {};
    std::unordered_map<std::string, std::shared_ptr<object>> objects = {};
  };
}

#include "scene.inl"
