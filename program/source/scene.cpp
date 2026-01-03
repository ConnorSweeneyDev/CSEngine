#include "scene.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/geometric.hpp"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "id.hpp"
#include "object.hpp"
#include "utility.hpp"

namespace cse
{
  scene::~scene()
  {
    additions.clear();
    removals.clear();
    hook.reset();
    objects.clear();
    camera.reset();
    parent.reset();
  }

  std::shared_ptr<scene> scene::remove_object(const help::id name)
  {
    if (!initialized)
    {
      if (auto iterator{objects.find(name)}; iterator != objects.end()) objects.erase(iterator);
      return shared_from_this();
    }
    if (objects.contains(name)) removals.insert(name);
    return shared_from_this();
  }

  void scene::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_initialize");
    if (!camera) throw exception("Scene must have a camera to be initialized");
    if (!camera->initialized) camera->initialize();
    for (const auto &[name, object] : objects)
      if (!removals.contains(name) && !object->initialized) object->initialize(instance, gpu);
    initialized = true;
    hook.call<void()>("post_initialize");
  }

  void scene::event(const SDL_Event &event)
  {
    hook.call<void(const SDL_Event &)>("pre_event", event);
    if (!camera->initialized) throw exception("Camera is not initialized");
    camera->event(event);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name))
      {
        if (!object->initialized) throw exception("Object is not initialized");
        object->event(event);
      }
    hook.call<void(const SDL_Event &)>("post_event", event);
  }

  void scene::input(const bool *keys)
  {
    hook.call<void(const bool *)>("pre_input", keys);
    camera->input(keys);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name)) object->input(keys);
    hook.call<void(const bool *)>("post_input", keys);
  }

  void scene::simulate(const double active_poll_rate)
  {
    hook.call<void(const float)>("pre_simulate", static_cast<float>(active_poll_rate));
    camera->simulate(active_poll_rate);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name)) object->simulate(active_poll_rate);
    hook.call<void(const float)>("post_simulate", static_cast<float>(active_poll_rate));
  }

  void scene::render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                     const double alpha, const float aspect_ratio, const float scale_factor)
  {
    hook.call<void()>("pre_render");
    auto matrices = camera->render(alpha, aspect_ratio, scale_factor);
    std::vector<std::shared_ptr<object>> render_order{};
    render_order.reserve(objects.size() - removals.size());
    for (const auto &[name, object] : objects)
    {
      if (removals.contains(name)) continue;
      object->state.translation.interpolate(alpha);
      render_order.emplace_back(object);
    }
    const auto &camera_position = camera->state.translation.interpolated;
    const auto camera_forward = glm::normalize(camera->state.forward.interpolated);
    std::sort(render_order.begin(), render_order.end(),
              [&camera_position, &camera_forward](const auto &left, const auto &right)
              {
                float depth_a = glm::dot(left->state.translation.interpolated - camera_position, camera_forward);
                float depth_b = glm::dot(right->state.translation.interpolated - camera_position, camera_forward);
                if (!equal(depth_a, depth_b, 1e-2f)) return depth_a > depth_b;
                return left->graphics.property.priority < right->graphics.property.priority;
              });
    for (const auto &object : render_order)
      object->render(gpu, command_buffer, render_pass, matrices.first, matrices.second, alpha, scale_factor);
    hook.call<void()>("post_render");
  }

  void scene::cleanup(SDL_GPUDevice *gpu)
  {
    hook.call<void()>("pre_cleanup");
    if (camera->initialized) camera->cleanup();
    for (const auto &[name, object] : objects)
      if (object->initialized) object->cleanup(gpu);
    initialized = false;
    hook.call<void()>("post_cleanup");
  }

  void scene::process_updates()
  {
    if (removals.empty() && additions.empty()) return;
    auto game{parent.lock()};
    for (const auto &name : removals)
      if (auto iterator{objects.find(name)}; iterator != objects.end())
      {
        const auto &object{iterator->second};
        if (initialized && object->initialized)
          if (game) object->cleanup(game->window->graphics.gpu);
        objects.erase(iterator);
      }
    removals.clear();
    if (additions.empty()) return;
    for (auto &[name, object] : additions)
    {
      objects.insert_or_assign(name, object);
      if (initialized && !object->initialized)
        if (game) object->initialize(game->window->graphics.instance, game->window->graphics.gpu);
    }
    additions.clear();
  }

  void scene::update_previous()
  {
    camera->previous.update(camera->state, camera->graphics);
    for (const auto &[name, object] : objects)
      if (!removals.contains(name)) object->previous.update(object->state, object->graphics);
  }
}
