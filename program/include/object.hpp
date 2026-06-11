#pragma once

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"

#include "core.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "state.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  protected:
    struct initial_state
    {
      const glm::dvec3 translation{};
      const double rotation{};
      const glm::dvec2 scale{};
      const bool collidable{};
      const int priority{};
    };
    struct initial_graphics
    {
      const help::object_graphics::shader shader{};
      const help::object_graphics::texture texture{};
      const help::object_graphics::render render{};
      const int priority{};
    };

  public:
    virtual ~object() = default;
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  protected:
    object(const initial_state &state_, const initial_graphics &graphics_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_previous();
    virtual void on_event(const SDL_Event &event);
    virtual void on_input(const bool *keys);
    virtual void on_simulate(const double tick);
    virtual void on_collide(const double tick);
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
    void collide(const double tick);
    void render(const double alpha);
    void destroy();
    void clean();

  public:
    cse::scene *scene{};
    cse::name name{};
    help::object_state state{};
    help::object_graphics graphics{};
  };
}
