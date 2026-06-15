#pragma once

#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class interface
  {
    friend class game;
    friend struct help::game_state;
    friend class scene;

  protected:
    struct initial_state
    {
      const glm::dvec2 translation{};
      const double rotation{};
      const glm::dvec2 scale{};
      const std::string text{};
      const int priority{};
    };
    struct initial_graphics
    {
      const help::interface_graphics::shader shader{};
      const help::interface_graphics::texture texture{};
      const help::interface_graphics::render render{};
      const help::interface_graphics::text text{};
      const int priority{};
    };

  public:
    virtual ~interface() = default;
    interface(const interface &) = delete;
    interface &operator=(const interface &) = delete;
    interface(interface &&) = delete;
    interface &operator=(interface &&) = delete;

  protected:
    interface(const initial_state &state_, const initial_graphics &graphics_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_previous();
    virtual void on_hover();
    virtual void on_unhover();
    virtual void on_press(const Uint8 button);
    virtual void on_release(const Uint8 button);
    virtual void on_click(const Uint8 button);
    virtual void on_scroll(const glm::dvec2 &delta);
    virtual void on_event(const SDL_Event &event);
    virtual void on_input(const bool *keys);
    virtual void on_simulate(const double tick);
    virtual void on_render(const double alpha);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const double tick);
    void render(const double alpha);
    void destroy();
    void clean();

  public:
    cse::game *game{};
    cse::scene *scene{};
    cse::name name{};
    help::interface_state state{};
    help::interface_graphics graphics{};
  };
}
