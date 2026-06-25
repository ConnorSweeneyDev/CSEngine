#pragma once

#include <optional>

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double2.hpp"

#include "core.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"
#include "temporal.hpp"

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
      const temporal<glm::dvec2> translation{};
      const temporal<double> rotation{};
      const temporal<glm::dvec2> scale{};
      const bool interactable{};
      const int priority{};
    };
    struct initial_graphics
    {
      const help::interface_graphics::shader shader{};
      const help::interface_graphics::texture texture{};
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
    virtual void on_synchronize();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void synchronize();
    void event(const SDL_Event &event);
    void simulate(const double tick);
    void destroy();
    void clean();

  public:
    cse::game *game{};
    std::optional<cse::scene *> scene{};
    cse::name name{};
    help::interface_state state{};
    help::interface_graphics graphics{};
  };
}
