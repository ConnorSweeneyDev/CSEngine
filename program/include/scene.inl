#pragma once

#include "scene.hpp"

#include <string>

#include "exception.hpp"
#include "id.hpp"

namespace cse::core
{
  template <typename camera_type, typename... camera_arguments> void scene::set_camera(camera_arguments &&...arguments)
  {
    camera = std::make_unique<camera_type>(std::forward<camera_arguments>(arguments)...);
  }

  template <typename object_type, typename... object_arguments>
  void scene::add_object(helper::id name, object_arguments &&...arguments)
  {
    if (objects.contains(name)) throw utility::exception("Tried to add duplicate object to scene");
    objects.emplace(name, std::make_shared<object_type>(std::forward<object_arguments>(arguments)...));
  }
}
