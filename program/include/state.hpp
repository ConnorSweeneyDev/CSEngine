#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_int2.hpp"

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
      std::vector<std::shared_ptr<cse::scene>> scenes{};
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
      SDL_DisplayID display{};
      int left{};
      int top{};
      unsigned int width{};
      unsigned int height{};
      bool running{};
      help::timer timer{};
      help::phase phase{};
    };
    struct active
    {
      property<SDL_DisplayID> display{};
      property<int> left{};
      property<int> top{};
      property<unsigned int> width{};
      property<unsigned int> height{};
      bool running{};
      help::timer timer{};
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

    void handle_move(const bool fullscreen, SDL_Window *instance);
    void handle_manual_move(const bool fullscreen, SDL_Window *instance, const int CENTER);
    void handle_manual_display_move(const bool fullscreen, SDL_Window *instance, const SDL_DisplayID PRIMARY);
    void handle_resize(const bool fullscreen, SDL_Window *instance, SDL_GPUDevice *gpu, SDL_GPUTexture *&depth_texture,
                       std::function<void(const unsigned int, const unsigned int, SDL_GPUDevice *, SDL_GPUTexture *&)>
                         generate_depth_texture);
    void
    handle_manual_resize(const bool fullscreen, SDL_Window *instance, SDL_GPUDevice *gpu,
                         SDL_GPUTexture *&depth_texture,
                         std::function<void(const unsigned int, const unsigned int, SDL_GPUDevice *, SDL_GPUTexture *&)>
                           generate_depth_texture);
    static glm::ivec2 calculate_display_center(const SDL_DisplayID display, const unsigned int width,
                                               const unsigned int height);
    static glm::ivec2 relative_to_absolute(const SDL_DisplayID display, const int left, const int top);
    static glm::ivec2 absolute_to_relative(const SDL_DisplayID display, const int left, const int top);

  public:
    window_state::previous previous{};
    window_state::active active{};

  private:
    SDL_Event event{};
    const bool *input{};
    int windowed_left{};
    int windowed_top{};
    unsigned int windowed_width{};
    unsigned int windowed_height{};
  };

  struct scene_state
  {
    friend class cse::game;
    friend class cse::scene;

  private:
    struct previous
    {
      std::shared_ptr<cse::camera> camera{};
      std::vector<std::shared_ptr<cse::object>> objects{};
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
    friend class cse::scene;
    friend class cse::object;
    friend struct scene_graphics;

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
}
