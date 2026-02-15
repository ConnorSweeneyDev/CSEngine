#pragma once

#include <functional>
#include <memory>

#include "glm/ext/vector_double4.hpp"

#include "declaration.hpp"
#include "enumeration.hpp"
#include "function.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class game : public std::enable_shared_from_this<game>
  {
  protected:
    struct hook : public enumeration<hook>
    {
      static inline const value PRE_PREPARE{};
      static inline const value POST_PREPARE{};
      static inline const value PRE_CREATE{};
      static inline const value POST_CREATE{};
      static inline const value PRE_SYNC{};
      static inline const value POST_SYNC{};
      static inline const value PRE_EVENT{};
      static inline const value POST_EVENT{};
      static inline const value PRE_INPUT{};
      static inline const value POST_INPUT{};
      static inline const value PRE_SIMULATE{};
      static inline const value POST_SIMULATE{};
      static inline const value PRE_RENDER{};
      static inline const value POST_RENDER{};
      static inline const value PRE_DESTROY{};
      static inline const value POST_DESTROY{};
      static inline const value PRE_CLEAN{};
      static inline const value POST_CLEAN{};
    };

  public:
    virtual ~game() = default;
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <trait::is_game game_type, typename... game_arguments> static std::shared_ptr<game_type>
    create(const std::function<void(const std::shared_ptr<game_type>)> &config, game_arguments &&...arguments);
    template <trait::is_callable callable, typename... game_arguments>
    static std::shared_ptr<game> create(callable &&config, game_arguments &&...arguments);
    template <trait::is_window window_type, typename... window_arguments> game &set(window_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &set(const name name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
              scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &set(const name name, callable &&config, scene_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &current(const name name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                  scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &current(const name name, callable &&config, scene_arguments &&...arguments);
    game &current(const name name);
    game &remove(const name name);

    void run();

  protected:
    game(const double poll_rate_, const double frame_rate_, const double aspect_ratio_, const glm::dvec4 &clear_color_);

  private:
    void prepare();
    void create();
    void previous();
    void sync();
    void event();
    void input();
    void simulate();
    void render();
    void destroy();
    void clean();

    bool running();
    void time();
    bool behind();
    bool ready();
    void fps();

  public:
    help::game_state state{};
    help::game_graphics graphics{};
    help::hooks hooks{};

  private:
    static inline std::weak_ptr<game> instance{};
  };
}

#include "game.inl" // IWYU pragma: keep
