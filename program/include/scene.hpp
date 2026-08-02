#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_events.h"

#include "collision.hpp"
#include "container.hpp"
#include "core.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "timer.hpp"

namespace cse::help::scene
{
  struct previous
  {
  public:
    previous() = default;
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    std::shared_ptr<cse::camera> camera{};
    help::container<cse::interface> interfaces{};
    help::container<cse::object> objects{};
    help::container<cse::light> lights{};
    std::vector<contact> contacts{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend class cse::game;
    friend class cse::scene;
    friend struct game::active;

  private:
    struct contact_key
    {
      struct hash
      {
        std::size_t operator()(const contact_key &key) const;
      };

      bool operator==(const contact_key &other) const = default;

      std::size_t self;
      std::size_t target;
      std::uint64_t self_hitbox;
      std::uint64_t target_hitbox;
    };

  public:
    active() = default;
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void synchronize(previous &last);
    void render(game::active &game_active, const double aspect, const double alpha);

    void generate_simulation_order();
    void generate_contacts();

    void generate_graphics_order(const double alpha);

  public:
    std::shared_ptr<cse::camera> camera{};
    help::container<cse::interface> interfaces{};
    help::container<cse::object> objects{};
    help::container<cse::light> lights{};
    std::vector<contact> contacts{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};

  private:
    std::unordered_set<cse::name> interface_removals{};
    help::container<cse::interface> interface_additions{};
    std::vector<cse::interface *> interface_simulation_order{};
    std::unordered_set<cse::name> object_removals{};
    help::container<cse::object> object_additions{};
    std::vector<cse::object *> object_simulation_order{};
    std::unordered_set<cse::name> light_removals{};
    help::container<cse::light> light_additions{};
    std::vector<cse::light *> light_simulation_order{};

    std::vector<cse::object *> object_graphics_order{};
    std::vector<cse::light *> light_graphics_order{};

    std::vector<collision::entry> contact_entries{};
    std::unordered_map<contact_key, std::size_t, contact_key::hash> contact_lookup{};
    std::vector<std::size_t> contact_sweep{};
  };

  struct next
  {
  public:
    std::optional<std::shared_ptr<cse::camera>> camera{};
  };
}

namespace cse
{
  class scene
  {
    friend class game;

  protected:
    struct initial
    {
    };

  public:
    virtual ~scene() = default;
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <trait::is_camera camera_type, typename... camera_arguments>
    camera_type &set(camera_arguments &&...arguments);
    template <trait::is_interface interface_type, typename... interface_arguments>
    interface_type &set(const cse::name interface_name, interface_arguments &&...arguments);
    template <trait::is_object object_type, typename... object_arguments>
    object_type &set(const cse::name object_name, object_arguments &&...arguments);
    template <trait::is_light light_type, typename... light_arguments>
    light_type &set(const cse::name light_name, light_arguments &&...arguments);
    template <typename... target_types>
      requires((sizeof...(target_types) == 0) ||
               ((std::is_void_v<target_types> || trait::is_object<target_types> || trait::is_light<target_types> ||
                 trait::is_interface<target_types>) &&
                ...))
    void remove(const cse::name target_name);

  protected:
    explicit scene() = default;
    virtual void pre_prepare();
    virtual void post_prepare();
    virtual void pre_create();
    virtual void post_create();
    virtual void pre_synchronize();
    virtual void post_synchronize();
    virtual void pre_event(const SDL_Event &event);
    virtual void post_event(const SDL_Event &event);
    virtual void pre_simulate(const double tick);
    virtual void post_simulate(const double tick);
    virtual void pre_collide(const double tick);
    virtual void post_collide(const double tick);
    virtual void pre_render(const double alpha);
    virtual void post_render(const double alpha);
    virtual void pre_destroy();
    virtual void post_destroy();
    virtual void pre_clean();
    virtual void post_clean();

  private:
    void prepare();
    void create();
    void synchronize();
    void event(const SDL_Event &event);
    void simulate(const double tick);
    void collide(const double tick);
    void render(const double aspect, const double alpha);
    void destroy();
    void clean();

  public:
    cse::game *game{};
    cse::identity name{};
    help::scene::previous previous{};
    help::scene::active active{};
    help::scene::next next{};
  };
}

#include "scene.inl" // IWYU pragma: keep
