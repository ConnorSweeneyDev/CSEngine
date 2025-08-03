#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "object.hpp"

namespace cse::base
{
  class scene
  {
  public:
    scene();
    virtual ~scene();

    template <typename camera_type, typename... camera_arguments> void set_camera(camera_arguments &&...arguments);
    template <typename object_type, typename... object_arguments>
    void add_object(const std::string &name, object_arguments &&...arguments);

    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void cleanup(SDL_GPUDevice *gpu);
    void input(const bool *key_state);
    void simulate(double simulation_alpha);
    void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, int width, int height);

  protected:
    std::function<void(const bool *key_state)> handle_input = nullptr;
    std::function<void(double simulation_alpha)> handle_simulate = nullptr;

  private:
    std::unique_ptr<camera> camera = nullptr;
    std::unordered_map<std::string, std::shared_ptr<object>> objects = {};
  };
}

#include "scene.inl"
