#include "scene.hpp"

#include <memory>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "exception.hpp"
#include "id.hpp"
#include "object.hpp"

namespace cse::core
{
  scene::~scene()
  {
    objects.clear();
    camera.reset();
    hooks.clear_all();
  }

  std::shared_ptr<class camera> scene::get_camera() const noexcept { return camera; }

  std::shared_ptr<class camera> scene::get_camera_strict() const
  {
    if (camera) return camera;
    throw utility::exception("Scene camera is not initialized");
  }

  std::shared_ptr<object> scene::get_object(const helper::id name) const noexcept
  {
    if (!objects.contains(name)) return nullptr;
    return objects.at(name);
  }

  std::shared_ptr<object> scene::get_object_strict(const helper::id name) const
  {
    if (!objects.contains(name)) throw cse::utility::exception("Requested object does not exist in scene");
    if (auto object{objects.at(name)}) return object;
    throw utility::exception("Requested object is not initialized");
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->initialize(instance, gpu);
  }

  void scene::event(const SDL_Event &event)
  {
    hooks.call<void(const SDL_Event &)>("event_pre", event);
    camera->event(event);
    for (const auto &object : objects) object.second->event(event);
    hooks.call<void(const SDL_Event &)>("event_post", event);
  }

  void scene::input(const bool *keys)
  {
    hooks.call<void(const bool *)>("input_pre", keys);
    camera->input(keys);
    for (const auto &object : objects) object.second->input(keys);
    hooks.call<void(const bool *)>("input_post", keys);
  }

  void scene::simulate(const double simulation_alpha)
  {
    hooks.call<void(const double)>("simulate_pre", simulation_alpha);
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
    hooks.call<void(const double)>("simulate_post", simulation_alpha);
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const float target_aspect_ratio, const float global_scale_factor)
  {
    auto matrices = camera->render(target_aspect_ratio, global_scale_factor);
    for (const auto &object : objects)
      object.second->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, global_scale_factor);
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    for (const auto &object : objects) object.second->cleanup(gpu);
  }
}
