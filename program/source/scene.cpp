#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "object.hpp"

namespace cse::core
{
  scene::~scene()
  {
    objects.clear();
    camera.reset();
    simulate_hooks.clear();
    input_hooks.clear();
    event_hooks.clear();
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->initialize(instance, gpu);
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->cleanup(gpu);
  }

  void scene::event(const SDL_Event &event)
  {
    event_hooks.call("pre", event);
    camera->event(event);
    for (const auto &object : objects) object.second->event(event);
    event_hooks.call("post", event);
  }

  void scene::input(const bool *keys)
  {
    input_hooks.call("pre", keys);
    camera->input(keys);
    for (const auto &object : objects) object.second->input(keys);
    input_hooks.call("post", keys);
  }

  void scene::simulate(const double simulation_alpha)
  {
    simulate_hooks.call("pre", simulation_alpha);
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
    simulate_hooks.call("post", simulation_alpha);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const float target_aspect_ratio, const float global_scale_factor)
  {
    auto matrices = camera->render(target_aspect_ratio, global_scale_factor);
    for (const auto &object : objects)
      object.second->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, global_scale_factor);
  }
}
