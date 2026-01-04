#pragma once

#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "name.hpp"
#include "property.hpp"
#include "transform.hpp"

namespace cse::help
{
  struct game_state
  {
    friend class cse::game;

  private:
    struct scene_reference
    {
      class name name{};
      std::shared_ptr<class scene> pointer{};
    };

    struct previous
    {
      std::shared_ptr<class window> window{};
      std::vector<help::name> scene_names{};
      struct scene_reference scene{};
      double poll_rate{};
    };
    struct active
    {
      std::weak_ptr<class game> parent{};
      std::shared_ptr<class window> window{};
      std::unordered_map<help::name, std::shared_ptr<class scene>> scenes{};
      struct scene_reference scene{};
      double poll_rate{};
    };
    struct next
    {
      std::optional<struct scene_reference> scene{};
    };

  public:
    game_state() = default;
    game_state(const double poll_rate_);
    ~game_state();
    game_state(const game_state &) = delete;
    game_state &operator=(const game_state &) = delete;
    game_state(game_state &&) = delete;
    game_state &operator=(game_state &&) = delete;

  public:
    void update_previous();

  public:
    struct previous previous{};
    struct active active{};
    struct next next{};

  private:
    double actual_poll_rate{1.0 / active.poll_rate};
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
      bool running{};
      unsigned int width{};
      unsigned int height{};
      int left{};
      int top{};
      SDL_DisplayID display_index{};
      bool fullscreen{};
      bool vsync{};
    };
    struct active
    {
      std::weak_ptr<class game> parent{};
      bool running{};
      property<unsigned int> width{};
      property<unsigned int> height{};
      property<int> left{};
      property<int> top{};
      property<SDL_DisplayID> display_index{};
      property<bool> fullscreen{};
      property<bool> vsync{};
    };

  public:
    window_state() = default;
    window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    ~window_state();
    window_state(const window_state &) = delete;
    window_state &operator=(const window_state &) = delete;
    window_state(window_state &&) = delete;
    window_state &operator=(window_state &&) = delete;

  private:
    void update_previous();

  public:
    struct previous previous{};
    struct active active{};

  private:
    bool initialized{};
    SDL_Event event{};
    const bool *keys{};
  };

  struct scene_state
  {
    friend class cse::game;
    friend class cse::scene;

  private:
    struct previous
    {
      std::shared_ptr<class camera> camera{};
      std::vector<help::name> object_names{};
    };
    struct active
    {
      std::weak_ptr<class game> parent{};
      std::shared_ptr<class camera> camera{};
      std::unordered_map<help::name, std::shared_ptr<class object>> objects{};
    };
    struct next
    {
      std::optional<std::shared_ptr<class camera>> camera{};
    };

  public:
    scene_state() = default;
    ~scene_state();
    scene_state(const scene_state &) = delete;
    scene_state &operator=(const scene_state &) = delete;
    scene_state(scene_state &&) = delete;
    scene_state &operator=(scene_state &&) = delete;

  private:
    void update_previous();

  public:
    struct previous previous{};
    struct active active{};
    struct next next{};

  private:
    bool initialized{};
    std::unordered_set<help::name> removals{};
    std::unordered_map<help::name, std::shared_ptr<object>> additions{};
  };

  struct camera_state
  {
    friend class cse::scene;
    friend class cse::camera;

  private:
    struct previous
    {
      transform_value translation{};
      transform_value forward{};
      transform_value up{};
    };
    struct active
    {
      std::weak_ptr<class scene> parent{};
      transform_value translation{};
      transform_value forward{};
      transform_value up{};
    };

  public:
    camera_state() = default;
    camera_state(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_);
    ~camera_state();
    camera_state(const camera_state &) = delete;
    camera_state &operator=(const camera_state &) = delete;
    camera_state(camera_state &&) = delete;
    camera_state &operator=(camera_state &&) = delete;

  private:
    glm::mat4 calculate_view_matrix() const;

    void update_previous();

  public:
    struct previous previous{};
    struct active active{};

  private:
    bool initialized{};
  };

  struct object_state
  {
    friend class cse::scene;
    friend class cse::object;

  private:
    struct previous
    {
      transform_value translation{};
      transform_value rotation{};
      transform_value scale{};
    };
    struct active
    {
      std::weak_ptr<class scene> parent{};
      transform_value translation{};
      transform_value rotation{};
      transform_value scale{};
    };

  public:
    object_state() = default;
    object_state(const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform_);
    ~object_state();
    object_state(const object_state &) = delete;
    object_state &operator=(const object_state &) = delete;
    object_state(object_state &&) = delete;
    object_state &operator=(object_state &&) = delete;

  private:
    glm::mat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height) const;

    void update_previous();

  public:
    struct previous previous{};
    struct active active{};

  private:
    bool initialized{};
  };
}
