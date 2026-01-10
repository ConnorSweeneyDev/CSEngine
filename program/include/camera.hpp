#pragma once

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "state.hpp"
#include "wrapper.hpp"

namespace cse
{
  class camera
  {
    friend class scene;

  protected:
    struct hook : public enumeration<hook>
    {
      using enumeration::enumeration;
      inline static const enumeration_value<hook> PREPARE{};
      inline static const enumeration_value<hook> CREATE{};
      inline static const enumeration_value<hook> EVENT{};
      inline static const enumeration_value<hook> INPUT{};
      inline static const enumeration_value<hook> SIMULATE{};
      inline static const enumeration_value<hook> RENDER{};
      inline static const enumeration_value<hook> DESTROY{};
      inline static const enumeration_value<hook> CLEAN{};
    };

  public:
    camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform_, const double fov_);
    virtual ~camera();
    camera(const camera &) = delete;
    camera &operator=(const camera &) = delete;
    camera(camera &&) = delete;
    camera &operator=(camera &&) = delete;

  private:
    void prepare();
    void create();
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const float poll_rate);
    std::pair<glm::mat4, glm::mat4> render(const double alpha, const float aspect_ratio);
    void destroy();
    void clean();

  public:
    help::camera_state state{};
    help::camera_graphics graphics{};
    help::hooks hooks{};
  };
}
