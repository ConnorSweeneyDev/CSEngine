#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"

#include "collision.hpp"
#include "core.hpp"
#include "input.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help
{
  enum class phase
  {
    CLEANED,
    PREPARED,
    CREATED
  };

  struct game_state
  {
    friend class cse::game;

  private:
    struct tick
    {
      double target{};
      unsigned int count{};
      double average{};
    };
    struct next_scene
    {
      cse::name name{};
      std::shared_ptr<cse::scene> pointer{};
    };

    struct previous
    {
      game_state::tick tick{};
      std::shared_ptr<cse::window> window{};
      std::vector<std::shared_ptr<cse::scene>> scenes{};
      std::shared_ptr<cse::scene> scene{};
      std::vector<std::shared_ptr<cse::interface>> interfaces{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      game_state::tick tick{};
      std::shared_ptr<cse::window> window{};
      std::vector<std::shared_ptr<cse::scene>> scenes{};
      std::shared_ptr<cse::scene> scene{};
      std::vector<std::shared_ptr<cse::interface>> interfaces{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct next
    {
      std::optional<std::shared_ptr<cse::window>> window{};
      std::optional<next_scene> scene{};
    };

  public:
    game_state() = default;
    game_state(const double tick_);
    ~game_state() = default;
    game_state(const game_state &) = delete;
    game_state &operator=(const game_state &) = delete;
    game_state(game_state &&) = delete;
    game_state &operator=(game_state &&) = delete;

  private:
    void update_previous();

    void generate_order(const std::vector<std::shared_ptr<interface>> &interfaces);
    void generate_pool(const std::vector<interface *> &interfaces);

    void reset_targets();
    bool inside(const glm::dvec2 &position, const double aspect, const unsigned int resolution);
    void interact(const SDL_Event &event, const double aspect, const unsigned int resolution);
    void hover(SDL_Window *instance, const double aspect, const unsigned int resolution);

  public:
    game_state::previous previous{};
    game_state::active active{};
    game_state::next next{};

  private:
    double actual_tick{1.0 / active.tick.target};
    double time{};
    double accumulator{};
    double alpha{};
    std::unordered_set<cse::name> interface_removals{};
    std::vector<std::shared_ptr<interface>> interface_additions{};
    std::vector<interface *> order{};
    std::vector<interface *> pool{};
  };

  struct window_state
  {
    friend class cse::game;
    friend class cse::window;

  private:
    struct viewport
    {
      double left{};
      double top{};
      double width{};
      double height{};
    };

    struct shadow
    {
      cse::mouse mouse{};
    };

    struct previous
    {
      SDL_DisplayID display{};
      int left{};
      int top{};
      unsigned int width{};
      unsigned int height{};
      bool running{};
      cse::keyboard keyboard{};
      cse::mouse mouse{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      SDL_DisplayID display{};
      int left{};
      int top{};
      unsigned int width{};
      unsigned int height{};
      bool running{};
      cse::keyboard keyboard{};
      cse::mouse mouse{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };

  public:
    window_state() = default;
    window_state(const SDL_DisplayID display_, const int left_, const int top_, const unsigned int width_,
                 const unsigned int height_);
    ~window_state() = default;
    window_state(const window_state &) = delete;
    window_state &operator=(const window_state &) = delete;
    window_state(window_state &&) = delete;
    window_state &operator=(window_state &&) = delete;

  private:
    void update_previous();

    void poll_mouse(SDL_Window *instance, const double aspect, const unsigned int resolution);
    void poll_keyboard();

    viewport letterbox(const unsigned int width, const unsigned int height, const double aspect);
    glm::dvec2 to_virtual(const double x, const double y, const unsigned int width, const unsigned int height,
                          const double aspect, const unsigned int resolution);
    glm::dvec2 to_pixel(const double x, const double y, const unsigned int width, const unsigned int height,
                        const double aspect, const unsigned int resolution);

  public:
    window_state::previous previous{};
    window_state::active active{};

  private:
    window_state::shadow shadow{};
    SDL_Event event{};
  };

  struct scene_state
  {
    friend class cse::game;
    friend class cse::scene;

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

    struct previous
    {
      std::shared_ptr<cse::camera> camera{};
      std::vector<std::shared_ptr<cse::object>> objects{};
      std::vector<std::shared_ptr<cse::interface>> interfaces{};
      std::vector<contact> contacts{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      std::shared_ptr<cse::camera> camera{};
      std::vector<std::shared_ptr<cse::object>> objects{};
      std::vector<std::shared_ptr<cse::interface>> interfaces{};
      std::vector<contact> contacts{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct next
    {
      std::optional<std::shared_ptr<cse::camera>> camera{};
    };

  public:
    scene_state() = default;
    ~scene_state() = default;
    scene_state(const scene_state &) = delete;
    scene_state &operator=(const scene_state &) = delete;
    scene_state(scene_state &&) = delete;
    scene_state &operator=(scene_state &&) = delete;

  private:
    void update_previous();

    void generate_order(const std::vector<std::shared_ptr<object>> &objects);
    void generate_order(const std::vector<std::shared_ptr<interface>> &interfaces);
    void generate_contacts();

  public:
    scene_state::previous previous{};
    scene_state::active active{};
    scene_state::next next{};

  private:
    std::unordered_set<cse::name> object_removals{};
    std::vector<std::shared_ptr<object>> object_additions{};
    std::vector<object *> object_order{};
    std::unordered_set<cse::name> interface_removals{};
    std::vector<std::shared_ptr<interface>> interface_additions{};
    std::vector<interface *> interface_order{};
  };

  struct camera_state
  {
    friend class cse::scene;
    friend class cse::camera;

  private:
    struct previous
    {
      temporal<glm::dvec3> translation{};
      temporal<glm::dvec3> forward{};
      temporal<glm::dvec3> up{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      temporal<glm::dvec3> translation{};
      temporal<glm::dvec3> forward{};
      temporal<glm::dvec3> up{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };

  public:
    camera_state() = default;
    camera_state(const glm::dvec3 &translation_, const glm::dvec3 &forward_, const glm::dvec3 &up_);
    ~camera_state() = default;
    camera_state(const camera_state &) = delete;
    camera_state &operator=(const camera_state &) = delete;
    camera_state(camera_state &&) = delete;
    camera_state &operator=(camera_state &&) = delete;

  private:
    void update_previous();

    glm::dmat4 calculate_view_matrix(const double alpha) const;

  public:
    camera_state::previous previous{};
    camera_state::active active{};
  };

  struct object_state
  {
    friend struct game_graphics;
    friend class cse::scene;
    friend class cse::object;

  private:
    struct previous
    {
      temporal<glm::dvec3> translation{};
      temporal<double> rotation{};
      temporal<glm::dvec2> scale{};
      bool collidable{};
      int priority{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      temporal<glm::dvec3> translation{};
      temporal<double> rotation{};
      temporal<glm::dvec2> scale{};
      bool collidable{};
      int priority{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };

  public:
    object_state() = default;
    object_state(const glm::dvec3 &translation_, const double rotation_, const glm::dvec2 &scale_,
                 const bool collidable_, const int priority_);
    ~object_state() = default;
    object_state(const object_state &) = delete;
    object_state &operator=(const object_state &) = delete;
    object_state(object_state &&) = delete;
    object_state &operator=(object_state &&) = delete;

  private:
    void update_previous();

    glm::dmat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                      const double alpha) const;

  public:
    object_state::previous previous{};
    object_state::active active{};
  };

  struct interface_state
  {
    friend class cse::game;
    friend struct game_graphics;
    friend class cse::scene;
    friend class cse::interface;

  private:
    struct target
    {
      cse::hitbox hovered{};
      std::array<cse::hitbox, SDL_BUTTON_X2 + 1> pressed{};
      std::array<cse::hitbox, SDL_BUTTON_X2 + 1> released{};
    };

    struct previous
    {
      temporal<glm::dvec2> translation{};
      temporal<double> rotation{};
      temporal<glm::dvec2> scale{};
      bool interactable{};
      int priority{};
      interface_state::target target{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };
    struct active
    {
      temporal<glm::dvec2> translation{};
      temporal<double> rotation{};
      temporal<glm::dvec2> scale{};
      bool interactable{};
      int priority{};
      interface_state::target target{};
      help::timer timer{};
      help::mixer mixer{};
      help::phase phase{};
    };

  public:
    interface_state() = default;
    interface_state(const glm::dvec2 &translation_, const double rotation_, const glm::dvec2 &scale_,
                    const bool interactable_, const int priority_);
    ~interface_state() = default;
    interface_state(const interface_state &) = delete;
    interface_state &operator=(const interface_state &) = delete;
    interface_state(interface_state &&) = delete;
    interface_state &operator=(interface_state &&) = delete;

  private:
    void update_previous();

    glm::dmat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                      const double alpha) const;
    glm::dmat4 calculate_text_matrix(const double width, const double height, const glm::dvec2 &offset,
                                     const double alpha) const;

  public:
    interface_state::previous previous{};
    interface_state::active active{};
  };
}
