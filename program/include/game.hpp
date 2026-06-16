#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double4.hpp"

#include "audio.hpp"
#include "core.hpp"
#include "function.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class game
  {
  protected:
    struct initial_state
    {
      const double tick{};
    };
    struct initial_graphics
    {
      const double frame{};
      const double aspect{};
      const unsigned int resolution{};
      const glm::dvec4 clear{};
    };
    struct initial_audio
    {
      const double master{};
      const double sound{};
      const double music{};
    };

  public:
    virtual ~game() = default;
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <trait::is_game game_type, typename... game_arguments> static std::shared_ptr<game_type>
    create(const std::function<void(const std::shared_ptr<game_type> &)> &config, game_arguments &&...arguments);
    template <trait::is_callable callable, typename... game_arguments>
    static std::shared_ptr<game> create(callable &&config, game_arguments &&...arguments);
    template <trait::is_window window_type, typename... window_arguments> game &set(window_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &set(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
              scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &set(const name scene_name, callable &&config, scene_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &current(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                  scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &current(const name scene_name, callable &&config, scene_arguments &&...arguments);
    game &current(const name scene_name);
    template <trait::is_interface interface_type, typename... interface_arguments>
    game &set(const name interface_name, interface_arguments &&...arguments);
    template <typename target_type = void>
      requires(std::is_void_v<target_type> || trait::is_scene<target_type> || trait::is_interface<target_type>)
    game &remove(const name target_name);

    void run();

  protected:
    game(const initial_state &state_, const initial_graphics &graphics_, const initial_audio &audio_);
    virtual void pre_prepare();
    virtual void post_prepare();
    virtual void pre_create();
    virtual void post_create();
    virtual void pre_previous();
    virtual void post_previous();
    virtual void pre_sync();
    virtual void post_sync();
    virtual void pre_event(const SDL_Event &event);
    virtual void post_event(const SDL_Event &event);
    virtual void pre_input();
    virtual void post_input();
    virtual void pre_simulate(const double tick);
    virtual void post_simulate(const double tick);
    virtual void pre_collide(const double tick);
    virtual void post_collide(const double tick);
    virtual void pre_render(const double alpha);
    virtual void post_render(const double alpha);
    virtual void pre_mix(const double alpha);
    virtual void post_mix(const double alpha);
    virtual void pre_destroy();
    virtual void post_destroy();
    virtual void pre_clean();
    virtual void post_clean();

  private:
    void prepare();
    void create();
    void previous();
    void sync();
    void event();
    void input();
    void simulate();
    void collide();
    void render();
    void mix();
    void destroy();
    void clean();

    void time();
    void step();

    bool running();
    bool behind();
    bool ready();

    void tps();
    void fps();

  public:
    help::game_state state{};
    help::game_graphics graphics{};
    help::game_audio audio{};

  private:
    static inline std::weak_ptr<game> instance{};
  };
}

#include "game.inl" // IWYU pragma: keep
