#pragma once

#include "scene.hpp"

#include <memory>
#include <string>
#include <tuple>

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "exception.hpp"
#include "id.hpp"

namespace cse::core
{
  template <typename camera_type> std::shared_ptr<camera_type> scene::get_camera() const noexcept
  {
    return std::static_pointer_cast<camera_type>(get_camera());
  }

  template <typename camera_type> std::shared_ptr<camera_type> scene::get_camera_strict() const
  {
    return std::static_pointer_cast<camera_type>(get_camera_strict());
  }

  template <typename camera_type, typename... camera_arguments>
  void scene::set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(transform, std::forward<camera_arguments>(arguments)...);
  }

  template <typename object_type> std::shared_ptr<object_type> scene::get_object(const helper::id name) const noexcept
  {
    return std::static_pointer_cast<object_type>(get_object(name));
  }

  template <typename object_type> std::shared_ptr<object_type> scene::get_object_strict(const helper::id name) const
  {
    return std::static_pointer_cast<object_type>(get_object_strict(name));
  }

  template <typename object_type, typename... object_arguments>
  void scene::add_object(const helper::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                         object_arguments &&...arguments)
  {
    if (objects.contains(name)) throw utility::exception("Tried to add duplicate object to scene");
    objects.emplace(name, std::make_shared<object_type>(transform, std::forward<object_arguments>(arguments)...));
  }
}
