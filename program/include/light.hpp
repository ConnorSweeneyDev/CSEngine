#pragma once

#include "SDL3/SDL_events.h"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"

#include "core.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help::light
{
  struct illumination
  {
    bool global{};
    temporal<glm::dvec4> brightness{{1.0, 1.0, 1.0, 1.0}};
    temporal<double> penetration{1.0};
    temporal<double> softness{};
    temporal<double> range{20};
    temporal<double> angle{360};
  };
  struct shadow
  {
    bool cast{true};
    temporal<double> darkness{1.0};
    temporal<double> softness{};
  };

  struct previous
  {
  public:
    previous() = default;
    previous(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec2> &rotation_,
             const light::illumination &illumination_, const light::shadow &shadow_, const int priority_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    temporal<glm::dvec3> translation{};
    temporal<glm::dvec2> rotation{};
    light::illumination illumination{};
    light::shadow shadow{};
    int priority{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend struct game::active;
    friend struct scene::active;
    friend class cse::scene;
    friend class cse::light;

  public:
    active() = default;
    active(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec2> &rotation_,
           const light::illumination &illumination_, const light::shadow &shadow_, const int priority_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void synchronize(previous &last);

    glm::dvec3 calculate_direction(const previous &last, const double alpha) const;

  public:
    temporal<glm::dvec3> translation{};
    temporal<glm::dvec2> rotation{};
    light::illumination illumination{};
    light::shadow shadow{};
    int priority{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };
}

namespace cse
{
  class light
  {
    friend class scene;

  protected:
    struct initial
    {
      temporal<glm::dvec3> translation{};
      temporal<glm::dvec2> rotation{};
      help::light::illumination illumination{};
      help::light::shadow shadow{};
      int priority{};
    };

  public:
    virtual ~light() = default;
    light(const light &) = delete;
    light &operator=(const light &) = delete;
    light(light &&) = delete;
    light &operator=(light &&) = delete;

  protected:
    light(const initial &initial_);
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
    cse::scene *scene{};
    cse::name name{};
    help::light::previous previous{};
    help::light::active active{};
  };
}
