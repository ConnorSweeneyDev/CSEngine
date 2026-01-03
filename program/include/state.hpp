#pragma once

#include "declaration.hpp"

#include <memory>
#include <optional>
#include <utility>

#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_uint2.hpp"

#include "id.hpp"
#include "property.hpp"
#include "transform.hpp"

namespace cse::help
{
  struct game_state
  {
    friend class cse::game;

  public:
    game_state() = default;
    game_state(const double poll_rate_);
    ~game_state();
    game_state(const game_state &) = delete;
    game_state &operator=(const game_state &) = delete;
    game_state(game_state &&) = delete;
    game_state &operator=(game_state &&) = delete;

  public:
    double poll_rate{};
    std::weak_ptr<class scene> scene{};
    std::optional<std::pair<help::id, std::shared_ptr<class scene>>> next_scene{};
  };

  struct window_state
  {
    friend class cse::window;

  public:
    window_state() = default;
    window_state(const glm::uvec2 &dimensions_, const bool fullscreen_, const bool vsync_);
    ~window_state() = default;
    window_state(const window_state &) = delete;
    window_state &operator=(const window_state &) = delete;
    window_state(window_state &&) = delete;
    window_state &operator=(window_state &&) = delete;

  public:
    bool running{};
    property<unsigned int> width{};
    property<unsigned int> height{};
    property<int> left{};
    property<int> top{};
    property<SDL_DisplayID> display_index{};
    property<bool> fullscreen{};
    property<bool> vsync{};
  };

  struct camera_state
  {
    friend class cse::camera;

  public:
    camera_state() = default;
    camera_state(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_);
    ~camera_state() = default;
    camera_state(const camera_state &) = delete;
    camera_state &operator=(const camera_state &) = delete;
    camera_state(camera_state &&) = delete;
    camera_state &operator=(camera_state &&) = delete;

  private:
    glm::mat4 calculate_view_matrix(const float scale_factor) const;

  public:
    transform_value translation{};
    transform_value forward{};
    transform_value up{};
  };

  struct object_state
  {
    friend class cse::object;

  public:
    object_state() = default;
    object_state(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_);
    ~object_state() = default;
    object_state(const object_state &) = delete;
    object_state &operator=(const object_state &) = delete;
    object_state(object_state &&) = delete;
    object_state &operator=(object_state &&) = delete;

  private:
    glm::mat4 calculate_model_matrix(const unsigned int frame_width, const unsigned int frame_height,
                                     const float scale_factor) const;

  public:
    transform_value translation{};
    transform_value rotation{};
    transform_value scale{};
  };
}
