#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "object.hpp"

namespace cse::core
{
  scene::scene() {}

  scene::~scene()
  {
    objects.clear();
    camera.reset();
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->initialize(instance, gpu);
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->cleanup(gpu);
  }

  void scene::input(const bool *keys)
  {
    if (handle_input) { handle_input(keys); }
    camera->input(keys);
    for (const auto &object : objects) object.second->input(keys);
  }

  void scene::simulate(const double simulation_alpha)
  {
    if (handle_simulate) { handle_simulate(simulation_alpha); }
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const unsigned int width, const unsigned int height, const float scale_factor)
  {
    auto matrices = camera->render(width, height, scale_factor);
    for (const auto &object : objects)
      object.second->render(gpu, command_buffer, render_pass, matrices[0], matrices[1], scale_factor);
  }
}
