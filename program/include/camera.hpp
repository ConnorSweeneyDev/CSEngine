#pragma once

#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double3.hpp"

#include "core.hpp"
#include "mixer.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help::camera
{
  struct clip
  {
    double near{};
    double far{};
  };

  struct previous
  {
  public:
    previous() = default;
    previous(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec3> &forward_,
             const temporal<glm::dvec3> &up_, const temporal<double> &fov_, const camera::clip &clip_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    temporal<glm::dvec3> translation{};
    temporal<glm::dvec3> forward{};
    temporal<glm::dvec3> up{};
    temporal<double> fov{};
    camera::clip clip{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend class cse::scene;
    friend class cse::camera;

  public:
    active() = default;
    active(const temporal<glm::dvec3> &translation_, const temporal<glm::dvec3> &forward_,
           const temporal<glm::dvec3> &up_, const temporal<double> &fov_, const camera::clip &clip_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void synchronize(previous &last);

    glm::dmat4 calculate_view_matrix(const previous &last, const double alpha) const;
    glm::dmat4 calculate_projection_matrix(const previous &last, const double aspect, const double alpha);

  public:
    temporal<glm::dvec3> translation{};
    temporal<glm::dvec3> forward{};
    temporal<glm::dvec3> up{};
    temporal<double> fov{};
    camera::clip clip{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };
}

namespace cse
{
  class camera
  {
    friend class scene;
    friend struct help::scene::active;

  protected:
    struct initial
    {
      const temporal<glm::dvec3> translation{};
      const temporal<glm::dvec3> forward{};
      const temporal<glm::dvec3> up{};
      const temporal<double> fov{};
      const help::camera::clip clip{};
    };

  public:
    virtual ~camera() = default;
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  protected:
    camera(const initial &initial_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_synchronize();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void on_render(const double alpha);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void synchronize();
    void event(const SDL_Event &event);
    void simulate(const double tick);
    std::pair<glm::dmat4, glm::dmat4> render(const double aspect, const double alpha);
    void destroy();
    void clean();

  public:
    cse::scene *scene{};
    help::camera::previous previous{};
    help::camera::active active{};
  };
}
