#include "scene.hpp"

#include <memory>
#include <string>
#include <utility>

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
}
