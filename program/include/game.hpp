#pragma once

#include <functional>
#include <memory>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double4.hpp"

#include "declaration.hpp"
#include "function.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class game : public std::enable_shared_from_this<game>
  {
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
    game(const double tick_, const double frame_, const double aspect_, const glm::dvec4 &clear_);
    virtual void pre_prepare() {}
    virtual void post_prepare() {}
    virtual void pre_create() {}
    virtual void post_create() {}
    virtual void pre_previous() {}
    virtual void post_previous() {}
    virtual void pre_sync() {}
    virtual void post_sync() {}
    virtual void pre_event(const SDL_Event &) {}
    virtual void post_event(const SDL_Event &) {}
    virtual void pre_input(const bool *) {}
    virtual void post_input(const bool *) {}
    virtual void pre_simulate(const double) {}
    virtual void post_simulate(const double) {}
    virtual void pre_render(const double) {}
    virtual void post_render(const double) {}
    virtual void pre_destroy() {}
    virtual void post_destroy() {}
    virtual void pre_clean() {}
    virtual void post_clean() {}

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

  private:
    static inline std::weak_ptr<game> instance{};
  };
}

#include "game.inl" // IWYU pragma: keep
