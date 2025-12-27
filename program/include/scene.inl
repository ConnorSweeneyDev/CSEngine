#pragma once

#include "scene.hpp"

#include <memory>
#include <tuple>

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "camera.hpp"
#include "game.hpp"
#include "id.hpp"

namespace cse
{
  template <typename camera_type, typename... camera_arguments>
  void scene::set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(transform, std::forward<camera_arguments>(arguments)...);
    camera->parent = shared_from_this();
  }

  template <typename object_type, typename... object_arguments>
  void scene::set_object(const help::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                         object_arguments &&...arguments)
  {
    if (initialized && objects.contains(name))
    {
      if (auto game{parent.lock()})
      {
        const auto &old_object{objects.at(name)};
        old_object->initialized ? old_object->cleanup(game->window->graphics.gpu) : void();
      }
      objects.erase(name);
    }

    auto object{std::make_shared<object_type>(transform, std::forward<object_arguments>(arguments)...)};
    object->parent = shared_from_this();
    objects.emplace(name, object);

    if (initialized)
      if (auto game{parent.lock()})
      {
        const auto &new_object{objects.at(name)};
        !new_object->initialized ? new_object->initialize(game->window->graphics.instance, game->window->graphics.gpu)
                                 : void();
      }
  }
}
