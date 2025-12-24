#pragma once

#include "scene.hpp"

#include <memory>
#include <tuple>

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "camera.hpp"
#include "id.hpp"

namespace cse
{
  template <typename camera_type, typename... camera_arguments>
  void scene::set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(transform, std::forward<camera_arguments>(arguments)...);
    camera->scene = shared_from_this();
  }

  template <typename object_type, typename... object_arguments>
  void scene::set_object(const help::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                         object_arguments &&...arguments)
  {
    auto object{std::make_shared<object_type>(transform, std::forward<object_arguments>(arguments)...)};
    object->scene = shared_from_this();
    objects.emplace(name, object);
  }
}

template <typename scene_type> std::shared_ptr<scene_type> as(const std::shared_ptr<cse::scene> &scene)
{
  return std::static_pointer_cast<scene_type>(scene);
}
