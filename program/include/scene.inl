#pragma once

#include "scene.hpp"

#include <memory>
#include <tuple>

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "camera.hpp"
#include "id.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_camera camera_type, typename... camera_arguments>
  void scene::set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(transform, std::forward<camera_arguments>(arguments)...);
    camera->parent = shared_from_this();
  }

  template <help::is_object object_type, typename... object_arguments>
  void scene::set_object(const help::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                         object_arguments &&...arguments)
  {
    if (objects.contains(name)) removals.insert(name);
    auto object{std::make_shared<object_type>(transform, std::forward<object_arguments>(arguments)...)};
    object->parent = shared_from_this();
    if (!initialized)
    {
      objects.insert_or_assign(name, object);
      return;
    }
    additions.insert_or_assign(name, object);
  }
}
