#pragma once

#include <tuple>
#include <utility>

#include "SDL3/SDL_events.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "state.hpp"
#include "wrapper.hpp"

namespace cse
{
  class camera
  {
    friend class scene;

  protected:
    struct hooks : public enumeration<hooks>
    {
      using enumeration::enumeration;
      static constexpr enumeration_value<hooks> PREPARE{};
      static constexpr enumeration_value<hooks> CREATE{};
      static constexpr enumeration_value<hooks> EVENT{};
      static constexpr enumeration_value<hooks> INPUT{};
      static constexpr enumeration_value<hooks> SIMULATE{};
      static constexpr enumeration_value<hooks> RENDER{};
      static constexpr enumeration_value<hooks> DESTROY{};
      static constexpr enumeration_value<hooks> CLEAN{};
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
    help::hook hook{};
  };
}
