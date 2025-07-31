#pragma once

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
    scene(const std::shared_ptr<camera> &custom_camera);
    virtual ~scene();

    void add_object(const std::string &name, const std::shared_ptr<object> &custom_object);

    virtual void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    virtual void cleanup(SDL_GPUDevice *gpu);

    virtual void input(const bool *key_state);
    virtual void simulate(double simulation_alpha);
    virtual void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, int width, int height);

  private:
    std::shared_ptr<camera> camera;
    std::unordered_map<std::string, std::shared_ptr<object>> objects = {};
  };
}
