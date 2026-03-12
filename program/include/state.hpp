#pragma once

#include <memory>
#include <optional>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_uint2.hpp"

#include "collision.hpp"
#include "core.hpp"
#include "name.hpp"
#include "property.hpp"
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
    struct next_scene
    {
      cse::name name{};
      std::shared_ptr<cse::scene> pointer{};
    };

    struct previous
    {
      double tick{};
      std::shared_ptr<cse::window> window{};
      std::unordered_set<name> scenes{};
      std::shared_ptr<cse::scene> scene{};
      help::timer timer{};
      help::phase phase{};
    };
    struct active
    {
      double tick{};
      std::shared_ptr<cse::window> window{};
      std::vector<std::shared_ptr<cse::scene>> scenes{};
      std::shared_ptr<cse::scene> scene{};
      help::timer timer{};
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

  public:
    game_state::previous previous{};
    game_state::active active{};
    game_state::next next{};

  private:
    double actual_tick{1.0 / active.tick};
    double time{};
    double accumulator{};
    double alpha{};
  };

  struct window_state
  {
    friend class cse::game;
    friend class cse::window;

  private:
    struct previous
    {
      unsigned int width{};
      unsigned int height{};
      bool fullscreen{};
      bool vsync{};
      int left{};
      int top{};
      SDL_DisplayID display_index{};
      bool running{};
      help::timer timer{};
      help::phase phase{};
    };
    struct active
    {
      property<unsigned int> width{};
      property<unsigned int> height{};
      property<bool> fullscreen{};
      property<bool> vsync{};
      property<int> left{};
      property<int> top{};
      property<SDL_DisplayID> display_index{};
      bool running{};
      help::timer timer{};
      help::phase phase{};
    };

  public:
    window_state() = default;
    window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    ~window_state() = default;
    window_state(const window_state &) = delete;
    window_state &operator=(const window_state &) = delete;
    window_state(window_state &&) = delete;
    window_state &operator=(window_state &&) = delete;

  private:
    void update_previous();

  public:
    window_state::previous previous{};
    window_state::active active{};

  private:
    SDL_Event event{};
    const bool *input{};
  };

  struct scene_state
  {
    friend class cse::game;
    friend class cse::scene;

  private:
    struct previous
    {
      std::shared_ptr<cse::camera> camera{};
      std::unordered_set<cse::name> objects{};
      std::vector<contact> contacts{};
      help::timer timer{};
      help::phase phase{};
    };
    struct active
    {
      std::shared_ptr<cse::camera> camera{};
      std::vector<std::shared_ptr<cse::object>> objects{};
      std::vector<contact> contacts{};
      help::timer timer{};
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
    void generate_contacts();

  public:
    scene_state::previous previous{};
    scene_state::active active{};
    scene_state::next next{};

  private:
    std::unordered_set<cse::name> removals{};
    std::vector<std::shared_ptr<object>> additions{};
    std::vector<object *> order{};
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
      help::phase phase{};
    };
    struct active
    {
      temporal<glm::dvec3> translation{};
      temporal<glm::dvec3> forward{};
      temporal<glm::dvec3> up{};
      help::timer timer{};
      help::phase phase{};
    };

  public:
    camera_state() = default;
    camera_state(const std::tuple<glm::dvec3, glm::dvec3, glm::dvec3> &transform_);
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
      help::phase phase{};
    };

  public:
    object_state() = default;
    object_state(const std::tuple<glm::dvec3, double, glm::dvec2> &transform_, const bool collidable_,
                 const int priority_);
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
}
