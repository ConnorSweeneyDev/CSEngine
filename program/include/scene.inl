#pragma once

#include "scene.hpp"

#include <string>

#include "exception.hpp"

namespace cse::core
{
  template <typename camera_type, typename... camera_arguments> void scene::set_camera(camera_arguments &&...arguments)
  {
    camera = std::make_unique<camera_type>(std::forward<camera_arguments>(arguments)...);
  }

  template <typename object_type, typename... object_arguments>
  void scene::add_object(const std::string &name, object_arguments &&...arguments)
  {
    if (objects.contains(name)) throw utility::exception("Object with name '{}' already exists", name);
    objects.emplace(name, std::make_shared<object_type>(std::forward<object_arguments>(arguments)...));
  }
}
