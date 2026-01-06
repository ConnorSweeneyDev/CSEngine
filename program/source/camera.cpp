#include "camera.hpp"

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "exception.hpp"

namespace cse
{
  camera::camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const double fov_)
    : state{transform_}, graphics{fov_}
  {
  }

  camera::~camera() { hook.reset(); }

  void camera::prepare()
  {
    if (state.prepared) throw exception("Camera cannot be prepared more than once");
    if (state.created) throw exception("Camera cannot be prepared while created");
    state.prepared = true;
    hook.call<void()>("prepare");
  }

  void camera::create()
  {
    if (!state.prepared) throw exception("Camera must be prepared before creation");
    if (state.created) throw exception("Camera cannot be created more than once");
    state.created = true;
    hook.call<void()>("create");
  }

  void camera::previous()
  {
    if (!state.prepared) throw exception("Camera must be prepared before updating previous state");
    if (!state.created) throw exception("Camera must be created before updating previous state");
    state.update_previous();
    graphics.update_previous();
  }

  void camera::event(const SDL_Event &event)
  {
    if (!state.prepared) throw exception("Camera must be prepared before processing events");
    if (!state.created) throw exception("Camera must be created before processing events");
    hook.call<void(const SDL_Event &)>("event", event);
  }

  void camera::input(const bool *input)
  {
    if (!state.prepared) throw exception("Camera must be prepared before processing input");
    if (!state.created) throw exception("Camera must be created before processing input");
    hook.call<void(const bool *)>("input", input);
  }

  void camera::simulate(const float poll_rate)
  {
    if (!state.prepared) throw exception("Camera must be prepared before simulation");
    if (!state.created) throw exception("Camera must be created before simulation");
    hook.call<void(const float)>("simulate", poll_rate);
  }

  std::pair<glm::mat4, glm::mat4> camera::render(const double alpha, const float aspect_ratio)
  {
    if (!state.prepared) throw exception("Camera must be prepared before rendering");
    if (!state.created) throw exception("Camera must be created before rendering");
    hook.call<void(const double)>("render", alpha);
    return {graphics.calculate_projection_matrix(alpha, aspect_ratio), state.calculate_view_matrix(alpha)};
  }

  void camera::destroy()
  {
    if (!state.prepared) throw exception("Camera must be prepared before destruction");
    if (!state.created) throw exception("Camera cannot be destroyed more than once");
    state.created = false;
    hook.call<void()>("destroy");
  }

  void camera::clean()
  {
    if (!state.prepared) throw exception("Camera cannot be cleaned more than once");
    if (state.created) throw exception("Camera must be destroyed before cleaning");
    state.prepared = false;
    hook.call<void()>("clean");
  }
}
