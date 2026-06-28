#pragma once

#include <array>
#include <optional>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_mouse.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double4.hpp"

#include "collision.hpp"
#include "core.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help::interface
{
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
    temporal<glm::dvec4> color{{1.0, 1.0, 1.0, 1.0}};
    temporal<double> transparency{1.0};
  };
  struct text
  {
    std::string content{};
    cse::font font{};
    unsigned int size{12};
    cse::style style{};
    temporal<glm::dvec4> color{{1.0, 1.0, 1.0, 1.0}};
    cse::align align{};
    double spacing{};
    bool wrap{};
    bool overflow{};
  };
  struct priority
  {
    int simulation{};
    int rendering{};
  };
  struct target
  {
    cse::hitbox hovered{};
    std::array<cse::hitbox, SDL_BUTTON_X2 + 1> pressed{};
    std::array<cse::hitbox, SDL_BUTTON_X2 + 1> clicked{};
  };

  struct previous
  {
  public:
    previous() = default;
    previous(const temporal<glm::dvec2> &translation_, const temporal<double> &rotation_,
             const temporal<glm::dvec2> &scale_, const bool interactable_, const interface::shader &shader_,
             const interface::texture &texture_, const interface::text &text_, const interface::priority &priority_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    temporal<glm::dvec2> translation{};
    temporal<double> rotation{};
    temporal<glm::dvec2> scale{};
    bool interactable{};
    interface::shader shader{};
    interface::texture texture{};
    interface::text text{};
    interface::priority priority{};

    interface::target target{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend class cse::game;
    friend struct game::active;
    friend class cse::scene;
    friend class cse::interface;

  public:
    active() = default;
    active(const temporal<glm::dvec2> &translation_, const temporal<double> &rotation_,
           const temporal<glm::dvec2> &scale_, const bool interactable_, const interface::shader &shader_,
           const interface::texture &texture_, const interface::text &text_, const interface::priority &priority_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void synchronize(previous &last);

    glm::dmat4 calculate_model_matrix(const previous &last, const unsigned int frame_width,
                                      const unsigned int frame_height, const double alpha) const;
    glm::dmat4 calculate_text_matrix(const previous &last, const double width, const double height,
                                     const glm::dvec2 &offset, const double alpha) const;
    void animate(const double tick);

  public:
    temporal<glm::dvec2> translation{};
    temporal<double> rotation{};
    temporal<glm::dvec2> scale{};
    bool interactable{};
    interface::shader shader{};
    interface::texture texture{};
    interface::text text{};
    interface::priority priority{};

    interface::target target{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };
}

namespace cse
{
  class interface
  {
    friend class game;
    friend class scene;

  protected:
    struct initial
    {
      const temporal<glm::dvec2> translation{};
      const temporal<double> rotation{};
      const temporal<glm::dvec2> scale{{1.0, 1.0}};
      const bool interactable{true};
      const help::interface::shader shader{};
      const help::interface::texture texture{};
      const help::interface::text text{};
      const help::interface::priority priority{};
    };

  public:
    virtual ~interface() = default;
    interface(const interface &) = delete;
    interface &operator=(const interface &) = delete;
    interface(interface &&) = delete;
    interface &operator=(interface &&) = delete;

  protected:
    interface(const initial &initial_);
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
    help::interface::previous previous{};
    help::interface::active active{};
  };
}
