#pragma once

#include "scene.hpp"

#include <memory>

#include "camera.hpp"
#include "exception.hpp"
#include "name.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_camera camera_type, typename... camera_arguments>
  std::shared_ptr<scene> scene::set_camera(camera_arguments &&...arguments)
  {
    if (camera && camera->initialized) throw exception("Tried to change camera after initialization");
    camera = std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...);
    camera->parent = weak_from_this();
    return shared_from_this();
  }

  template <help::is_object object_type, typename... object_arguments>
  std::shared_ptr<scene> scene::set_object(const help::name name, object_arguments &&...arguments)
  {
    if (objects.contains(name)) removals.insert(name);
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->parent = weak_from_this();
    if (!initialized)
    {
      objects.insert_or_assign(name, object);
      return shared_from_this();
    }
    additions.insert_or_assign(name, object);
    return shared_from_this();
  }
}
