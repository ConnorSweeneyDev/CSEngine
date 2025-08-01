#include "scene.hpp"

#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

#include "camera.hpp"
#include "exception.hpp"
#include "object.hpp"

namespace cse::base
{
  scene::scene(std::unique_ptr<base::camera> custom_camera) : camera(std::move(custom_camera))
  {
    if (!camera) throw utility::exception("Scene must have a camera");
  }

  scene::~scene()
  {
    objects.clear();
    camera.reset();
  }

  void scene::add_object(const std::string &name, std::unique_ptr<object> custom_object)
  {
    if (!custom_object) throw utility::exception("Cannot add a null object with name '{}'", name);
    if (objects.find(name) != objects.end()) throw utility::exception("Object with name '{}' already exists", name);
    objects[name] = std::move(custom_object);
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
    camera->input(key_state);
    for (const auto &object : objects) object.second->input(key_state);
  }

  void scene::simulate(double simulation_alpha)
  {
    camera->simulate(simulation_alpha);
    for (const auto &object : objects) object.second->simulate(simulation_alpha);
  }

  void scene::render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, int width, int height)
  {
    camera->render(width, height);
    for (const auto &object : objects)
      object.second->render(command_buffer, render_pass, camera->graphics.projection_matrix,
                            camera->graphics.view_matrix);
  }
}
