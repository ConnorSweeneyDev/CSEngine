#pragma once

#include "scene.hpp"

#include <string>
#include <tuple>

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "exception.hpp"
#include "id.hpp"

namespace cse::core
{
  template <typename camera_type, typename... camera_arguments>
  void scene::set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments)
  {
    camera = std::make_unique<camera_type>(transform, std::forward<camera_arguments>(arguments)...);
  }

  template <typename object_type, typename... object_arguments>
  void scene::add_object(const helper::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                         object_arguments &&...arguments)
  {
    if (objects.contains(name)) throw utility::exception("Tried to add duplicate object to scene");
    objects.emplace(name, std::make_shared<object_type>(transform, std::forward<object_arguments>(arguments)...));
  }
}
