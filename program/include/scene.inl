#pragma once

#include "scene.hpp"

#include <functional>
#include <memory>

#include "camera.hpp"
#include "id.hpp"
#include "traits.hpp"

namespace cse
{
  template <help::is_camera camera_type, typename... camera_arguments>
  std::shared_ptr<scene> scene::set_camera(camera_arguments &&...arguments)
  {
    return set_camera<camera_type>({}, std::forward<camera_arguments>(arguments)...);
  }

  template <help::is_camera camera_type, typename... camera_arguments>
  std::shared_ptr<scene> scene::set_camera(const std::function<void(const std::shared_ptr<camera_type>)> &config,
                                           camera_arguments &&...arguments)
  {
    camera = std::make_shared<camera_type>(std::forward<camera_arguments>(arguments)...);
    camera->parent = weak_from_this();
    if (config) config(std::static_pointer_cast<camera_type>(camera));
    return shared_from_this();
  }

  template <help::is_callable callable, typename... camera_arguments>
  std::shared_ptr<scene> scene::set_camera(callable &&config, camera_arguments &&...arguments)
  {
    using camera_type = typename help::type_from_callable<callable>::extracted_type;
    return set_camera<camera_type>(
      std::function<void(const std::shared_ptr<camera_type>)>(std::forward<callable>(config)),
      std::forward<camera_arguments>(arguments)...);
  }

  template <help::is_object object_type, typename... object_arguments>
  std::shared_ptr<scene> scene::set_object(const help::id name, object_arguments &&...arguments)
  {
    return set_object<object_type>(name, {}, std::forward<object_arguments>(arguments)...);
  }

  template <help::is_object object_type, typename... object_arguments>
  std::shared_ptr<scene> scene::set_object(const help::id name,
                                           const std::function<void(const std::shared_ptr<object_type>)> &config,
                                           object_arguments &&...arguments)
  {
    if (objects.contains(name)) removals.insert(name);
    auto object{std::make_shared<object_type>(std::forward<object_arguments>(arguments)...)};
    object->parent = weak_from_this();
    if (config) config(object);
    if (!initialized)
    {
      objects.insert_or_assign(name, object);
      return shared_from_this();
    }
    additions.insert_or_assign(name, object);
    return shared_from_this();
  }

  template <help::is_callable callable, typename... object_arguments>
  std::shared_ptr<scene> scene::set_object(const help::id name, callable &&config, object_arguments &&...arguments)
  {
    using object_type = typename help::type_from_callable<callable>::extracted_type;
    return set_object<object_type>(
      name, std::function<void(const std::shared_ptr<object_type>)>(std::forward<callable>(config)),
      std::forward<object_arguments>(arguments)...);
  }
}
