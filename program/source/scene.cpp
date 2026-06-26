#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double3.hpp"
#include "glm/geometric.hpp"

#include "camera.hpp"
#include "collision.hpp"
#include "container.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "interface.hpp"
#include "name.hpp"
#include "numeric.hpp"
#include "object.hpp"

namespace cse::help::scene
{
  std::size_t active::contact_key::hash::operator()(const contact_key &key) const
  {
    std::size_t seed{std::hash<std::size_t>{}(key.self)};
    const auto mix{[&seed](std::size_t value) { seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2); }};
    mix(std::hash<std::size_t>{}(key.target));
    mix(std::hash<std::uint64_t>{}(key.self_hitbox));
    mix(std::hash<std::uint64_t>{}(key.target_hitbox));
    return seed;
  }

  void active::synchronize(previous &last)
  {
    last.camera = camera;
    last.objects = objects;
    last.interfaces = interfaces;
    last.contacts = contacts;
    last.timer = timer;
    last.mixer = mixer;
    last.phase = phase;

    for (auto &[name, sound] : mixer.sounds)
    {
      sound.speed.instant = false;
      sound.volume.instant = false;
    }
    for (auto &[name, music] : mixer.musics)
    {
      music.speed.instant = false;
      music.volume.instant = false;
    }
  }

  void active::render(game::active &active, const double aspect, const double alpha)
  {
    generate_graphics_order(alpha);
    active.generate_object_samples_and_batches(object_graphics_order);
    active.upload_samples();
    active.draw_batches(camera->render(aspect, alpha));
  }

  void active::generate_simulation_order()
  {
    object_simulation_order.clear();
    for (object_simulation_order.reserve(objects.size()); const auto &object : objects)
      object_simulation_order.emplace_back(object.get());
    std::sort(object_simulation_order.begin(), object_simulation_order.end(),
              [](const cse::object *left, const cse::object *right)
              {
                if (left->active.priority.simulation != right->active.priority.simulation)
                  return left->active.priority.simulation > right->active.priority.simulation;
                return left->name.identifier() < right->name.identifier();
              });

    interface_simulation_order.clear();
    for (interface_simulation_order.reserve(interfaces.size()); const auto &interface : interfaces)
      interface_simulation_order.emplace_back(interface.get());
    std::sort(interface_simulation_order.begin(), interface_simulation_order.end(),
              [](const cse::interface *left, const cse::interface *right)
              {
                if (left->active.priority.simulation != right->active.priority.simulation)
                  return left->active.priority.simulation > right->active.priority.simulation;
                return left->name.identifier() < right->name.identifier();
              });
  }

  void active::generate_contacts()
  {
    contacts.clear();
    if (objects.empty()) return;

    static std::vector<collision::entry> entries{};
    entries.clear();
    entries.reserve(objects.size() * 4);
    for (std::size_t index{}; index < objects.size(); ++index)
    {
      const auto &object{objects[index]};
      auto object_hitboxes{collision::hitboxes(object.get())};
      if (object_hitboxes.empty()) continue;
      auto z{static_cast<std::int64_t>(std::floor(object->active.translation.value.z + 0.5))};
      for (const auto &[hitbox, rectangle] : object_hitboxes)
        entries.push_back({index, z, hitbox, collision::bounds(object.get(), rectangle)});
    }
    if (entries.empty()) return;

    auto comparator{[](const collision::entry &a, const collision::entry &b)
                    {
                      if (a.z != b.z) return a.z < b.z;
                      return a.bounds.left < b.bounds.left;
                    }};
    bool large_disorder{};
    for (std::size_t index{1}; index < entries.size(); ++index)
      if (comparator(entries[index], entries[index - 1]))
      {
        large_disorder = true;
        break;
      }

    if (large_disorder) { std::sort(entries.begin(), entries.end(), comparator); }
    else
      for (std::size_t first{1}; first < entries.size(); ++first)
      {
        auto key{entries[first]};
        std::size_t second{first};
        while (second > 0 && comparator(key, entries[second - 1]))
        {
          entries[second] = entries[second - 1];
          --second;
        }
        entries[second] = key;
      }

    static std::unordered_map<contact_key, std::size_t, contact_key::hash> contact_lookup{};
    contact_lookup.clear();
    const auto emit{[&](std::size_t self_index, std::size_t target_index, const auto &own, const auto &theirs,
                        const auto &self_bounds, const auto &target_bounds)
                    {
                      auto contact{collision::describe(objects[self_index]->name, objects[target_index].get(), own,
                                                       theirs, self_bounds, target_bounds)};
                      const double area{std::max(contact.overlap.x, 0.0) * std::max(contact.overlap.y, 0.0)};
                      const contact_key key{self_index, target_index, own.identifier(), theirs.identifier()};
                      const auto found{contact_lookup.find(key)};
                      if (found == contact_lookup.end())
                      {
                        contact_lookup.emplace(key, contacts.size());
                        contacts.push_back(std::move(contact));
                        return;
                      }
                      auto &existing{contacts[found->second]};
                      const double existing_area{std::max(existing.overlap.x, 0.0) * std::max(existing.overlap.y, 0.0)};
                      if (area > existing_area) existing = std::move(contact);
                    }};

    static std::vector<std::size_t> active_list{};
    active_list.clear();
    active_list.reserve(std::min(entries.size(), static_cast<std::size_t>(64)));
    std::size_t start{0};
    while (start < entries.size())
    {
      auto z{entries[start].z};
      std::size_t end{start + 1};
      while (end < entries.size() && entries[end].z == z) ++end;

      active_list.clear();
      for (std::size_t first{start}; first < end; ++first)
      {
        const auto &current{entries[first]};
        for (std::size_t second{}; second < active_list.size();)
        {
          const auto &other{entries[active_list[second]]};
          if (other.bounds.right < current.bounds.left)
          {
            active_list[second] = active_list.back();
            active_list.pop_back();
            continue;
          }
          if (current.index != other.index && collision::overlaps(current.bounds, other.bounds))
          {
            emit(current.index, other.index, current.hitbox, other.hitbox, current.bounds, other.bounds);
            emit(other.index, current.index, other.hitbox, current.hitbox, other.bounds, current.bounds);
          }
          ++second;
        }
        active_list.push_back(first);
      }
      start = end;
    }
  }

  void active::generate_graphics_order(const double alpha)
  {
    object_graphics_order.clear();
    for (object_graphics_order.reserve(objects.size()); const auto &object : objects)
      object_graphics_order.emplace_back(object.get());
    auto camera_translation = camera->active.translation.interpolated(camera->previous.translation, alpha);
    auto camera_forward = glm::normalize(camera->active.forward.interpolated(camera->previous.forward, alpha));
    std::sort(
      object_graphics_order.begin(), object_graphics_order.end(),
      [alpha, &camera_translation, &camera_forward](const auto &left, const auto &right)
      {
        const auto rendered = [](glm::dvec3 vector)
        { return glm::dvec3{std::floor(vector.x + 0.5), std::floor(vector.y + 0.5), std::floor(vector.z + 0.5)}; };
        double left_depth = glm::dot(
          rendered(left->active.translation.interpolated(left->previous.translation, alpha)) - camera_translation,
          camera_forward);
        double right_depth = glm::dot(
          rendered(right->active.translation.interpolated(right->previous.translation, alpha)) - camera_translation,
          camera_forward);
        if (!equal(left_depth, right_depth, 1e-4)) return left_depth > right_depth;
        if (left->active.priority.rendering != right->active.priority.rendering)
          return left->active.priority.rendering < right->active.priority.rendering;
        const auto left_batch{std::make_tuple(left->active.shader.vertex.data.data(),
                                              left->active.shader.fragment.data.data(),
                                              left->active.texture.image.data.data())};
        const auto right_batch{std::make_tuple(right->active.shader.vertex.data.data(),
                                               right->active.shader.fragment.data.data(),
                                               right->active.texture.image.data.data())};
        if (left_batch != right_batch) return left_batch < right_batch;
        return left->name.identifier() < right->name.identifier();
      });
  }
}

namespace cse
{
  void scene::pre_prepare() {}
  void scene::post_prepare() {}
  void scene::prepare()
  {
    if (active.phase != help::phase::CLEANED)
      throw exception("Scene '{}' must be cleaned before preparation", name.string());
    pre_prepare();
    if (!active.camera) throw exception("Scene '{}' must have a camera to be prepared", name.string());
    active.camera->prepare();
    for (const auto &object : active.objects) object->prepare();
    for (const auto &interface : active.interfaces) interface->prepare();
    active.phase = help::phase::PREPARED;
    post_prepare();
  }

  void scene::pre_create() {}
  void scene::post_create() {}
  void scene::create()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Scene '{}' must be prepared before creation", name.string());
    pre_create();
    active.camera->create();
    for (const auto &object : active.objects) object->create();
    for (const auto &interface : active.interfaces) interface->create();
    active.phase = help::phase::CREATED;
    post_create();
  }

  void scene::pre_synchronize() {}
  void scene::post_synchronize() {}
  void scene::synchronize()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before synchronization", name.string());
    pre_synchronize();
    active.synchronize(previous);
    if (next.camera.has_value())
    {
      auto &new_camera{next.camera.value()};
      active.camera->destroy();
      active.camera->clean();
      active.camera = new_camera;
      new_camera->prepare();
      new_camera->create();
      next.camera.reset();
    }
    active.camera->synchronize();
    if (!active.object_removals.empty())
    {
      for (const auto &object_name : active.object_removals)
        if (auto iterator{try_iterate(active.objects, object_name)}; iterator != active.objects.end())
        {
          const auto &object{*iterator};
          if (object->active.phase == help::phase::CREATED) object->destroy();
          object->clean();
          active.objects.erase(iterator);
        }
      active.object_removals.clear();
    }
    if (!active.object_additions.empty())
    {
      for (auto &object : active.object_additions)
      {
        set_or_add(active.objects, object);
        object->prepare();
        object->create();
      }
      active.object_additions.clear();
    }
    for (const auto &object : active.objects) object->synchronize();
    if (!active.interface_removals.empty())
    {
      for (const auto &interface_name : active.interface_removals)
        if (auto iterator{try_iterate(active.interfaces, interface_name)}; iterator != active.interfaces.end())
        {
          const auto &interface{*iterator};
          if (interface->active.phase == help::phase::CREATED) interface->destroy();
          interface->clean();
          active.interfaces.erase(iterator);
        }
      active.interface_removals.clear();
    }
    if (!active.interface_additions.empty())
    {
      for (auto &interface : active.interface_additions)
      {
        set_or_add(active.interfaces, interface);
        interface->prepare();
        interface->create();
      }
      active.interface_additions.clear();
    }
    active.generate_simulation_order();
    for (const auto &interface : active.interfaces) interface->synchronize();
    post_synchronize();
  }

  void scene::pre_event(const SDL_Event &) {}
  void scene::post_event(const SDL_Event &) {}
  void scene::event(const SDL_Event &event)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before processing events", name.string());
    pre_event(event);
    active.camera->event(event);
    for (const auto &object : active.object_simulation_order) object->event(event);
    for (const auto &interface : active.interface_simulation_order) interface->event(event);
    post_event(event);
  }

  void scene::pre_simulate(const double) {}
  void scene::post_simulate(const double) {}
  void scene::simulate(const double tick)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before simulation", name.string());
    pre_simulate(tick);
    active.timer.update(tick);
    active.camera->simulate(tick);
    for (const auto &object : active.object_simulation_order) object->simulate(tick);
    for (const auto &interface : active.interface_simulation_order) interface->simulate(tick);
    post_simulate(tick);
  }

  void scene::pre_collide(const double) {}
  void scene::post_collide(const double) {}
  void scene::collide(const double tick)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before collision", name.string());
    pre_collide(tick);
    for (active.generate_contacts(); const auto &object : active.object_simulation_order) object->collide(tick);
    post_collide(tick);
  }

  void scene::pre_render(const double) {}
  void scene::post_render(const double) {}
  void scene::render(const double aspect, const double alpha)
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before rendering", name.string());
    pre_render(alpha);
    active.render(game->active, aspect, alpha);
    post_render(alpha);
  }

  void scene::pre_destroy() {}
  void scene::post_destroy() {}
  void scene::destroy()
  {
    if (active.phase != help::phase::CREATED)
      throw exception("Scene '{}' must be created before destruction", name.string());
    pre_destroy();
    for (const auto &interface : active.interface_simulation_order) interface->destroy();
    for (const auto &object : active.object_simulation_order) object->destroy();
    active.camera->destroy();
    active.phase = help::phase::PREPARED;
    post_destroy();
  }

  void scene::pre_clean() {}
  void scene::post_clean() {}
  void scene::clean()
  {
    if (active.phase != help::phase::PREPARED)
      throw exception("Scene '{}' must be prepared before cleaning", name.string());
    pre_clean();
    for (const auto &interface : active.interface_simulation_order) interface->clean();
    for (const auto &object : active.object_simulation_order) object->clean();
    active.camera->clean();
    active.phase = help::phase::CLEANED;
    post_clean();
  }
}
