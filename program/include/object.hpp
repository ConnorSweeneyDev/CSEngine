#pragma once

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"

#include "core.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help::object
{
  struct priority
  {
    int simulation{};
    int rendering{};
  };
  struct shader
  {
    cse::vertex vertex{};
    cse::fragment fragment{};
  };
  struct texture
  {
    cse::image image{};
    cse::animation animation{};
    cse::playback playback{};
    cse::flip flip{};
    temporal<cse::color> color{};
    temporal<cse::transparency> transparency{};
  };

  struct previous
  {
  public:
    previous() = default;
    previous(const temporal<glm::dvec3> &translation_, const temporal<double> &rotation_,
             const temporal<glm::dvec2> &scale_, const bool collidable_, const object::shader &shader_,
             const object::texture &texture_, const object::priority &priority_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    temporal<glm::dvec3> translation{};
    temporal<double> rotation{};
    temporal<glm::dvec2> scale{};
    bool collidable{};
    object::shader shader{};
    object::texture texture{};
    object::priority priority{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend struct game::active;
    friend class cse::scene;
    friend class cse::object;

  public:
    active() = default;
    active(const temporal<glm::dvec3> &translation_, const temporal<double> &rotation_,
           const temporal<glm::dvec2> &scale_, const bool collidable_, const object::shader &shader_,
           const object::texture &texture_, const object::priority &priority_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void synchronize(previous &last);

    glm::dmat4 calculate_model_matrix(const previous &last, const unsigned int frame_width,
                                      const unsigned int frame_height, const double alpha) const;
    void animate(const double tick);

  public:
    temporal<glm::dvec3> translation{};
    temporal<double> rotation{};
    temporal<glm::dvec2> scale{};
    bool collidable{};
    object::shader shader{};
    object::texture texture{};
    object::priority priority{};

    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };
}

namespace cse
{
  class object
  {
    friend class scene;

  protected:
    struct initial
    {
      const temporal<glm::dvec3> translation{};
      const temporal<double> rotation{};
      const temporal<glm::dvec2> scale{};
      const bool collidable{};
      const help::object::shader shader{};
      const help::object::texture texture{};
      const help::object::priority priority{};
    };

  public:
    virtual ~object() = default;
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  protected:
    object(const initial &initial_);
    virtual void on_prepare();
    virtual void on_create();
    virtual void on_synchronize();
    virtual void on_event(const SDL_Event &event);
    virtual void on_simulate(const double tick);
    virtual void on_collide(const double tick);
    virtual void on_destroy();
    virtual void on_clean();

  private:
    void prepare();
    void create();
    void synchronize();
    void event(const SDL_Event &event);
    void simulate(const double tick);
    void collide(const double tick);
    void destroy();
    void clean();

  public:
    cse::scene *scene{};
    cse::name name{};
    help::object::previous previous{};
    help::object::active active{};
  };
}
