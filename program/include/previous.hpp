#pragma once

#include "declaration.hpp"
#include "graphics.hpp"
#include "state.hpp"

namespace cse::help
{
  struct game_previous
  {
    friend class cse::game;

  public:
    game_previous();
    game_previous(const game_state &state_, const game_graphics &graphics_);
    ~game_previous() = default;
    game_previous(const game_previous &) = delete;
    game_previous &operator=(const game_previous &) = delete;
    game_previous(game_previous &&) = delete;
    game_previous &operator=(game_previous &&) = delete;

  private:
    void update(const game_state &new_state, const game_graphics &new_graphics);

  public:
    game_state state{};
    game_graphics graphics{};
  };

  struct window_previous
  {
    friend class cse::game;

  public:
    window_previous();
    window_previous(const window_state &state_, const window_graphics &graphics_);
    ~window_previous() = default;
    window_previous(const window_previous &) = delete;
    window_previous &operator=(const window_previous &) = delete;
    window_previous(window_previous &&) = delete;
    window_previous &operator=(window_previous &&) = delete;

  private:
    void clean();
    void update(const window_state &new_state, const window_graphics &new_graphics);

  public:
    window_state state{};
    window_graphics graphics{};
  };

  struct camera_previous
  {
    friend class cse::scene;

  public:
    camera_previous();
    camera_previous(const camera_state &state_, const camera_graphics &graphics_);
    ~camera_previous() = default;
    camera_previous(const camera_previous &) = delete;
    camera_previous &operator=(const camera_previous &) = delete;
    camera_previous(camera_previous &&) = delete;
    camera_previous &operator=(camera_previous &&) = delete;

  private:
    void update(const camera_state &new_state, const camera_graphics &new_graphics);

  public:
    camera_state state{};
    camera_graphics graphics{};
  };

  struct object_previous
  {
    friend class cse::scene;

  public:
    object_previous();
    object_previous(const object_state &state_, const object_graphics &graphics_);
    ~object_previous() = default;
    object_previous(const object_previous &) = delete;
    object_previous &operator=(const object_previous &) = delete;
    object_previous(object_previous &&) = delete;

  private:
    void clean();
    void update(const object_state &new_state, const object_graphics &new_graphics);

  public:
    object_state state{};
    object_graphics graphics{};
  };
}
