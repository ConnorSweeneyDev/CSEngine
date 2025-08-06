#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "object.hpp"

namespace cse::base
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

  void scene::input(const bool *key_state)
  {
    if (handle_input) { handle_input(key_state); }
    camera->input(key_state);
    for (const auto &object : objects) object.second->input(key_state);
  }

  void scene::simulate(double simulation_alpha)
  {
    if (handle_simulate) { handle_simulate(simulation_alpha); }
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
  }

  void scene::render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, int width, int height)
  {
    auto matrices = camera->render(width, height);
    for (const auto &object : objects) object.second->render(command_buffer, render_pass, matrices[0], matrices[1]);
  }
}
