#pragma once

#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"

#include "declaration.hpp"
#include "graphics.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "state.hpp"

namespace cse
{
  class object
  {
    friend class scene;

  public:
    virtual ~object() = default;
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = delete;
    object &operator=(object &&) = delete;

  protected:
    object(const std::tuple<glm::ivec3, int, glm::ivec2> &transform_, const bool collidable_,
           const std::pair<vertex, fragment> &shader_,
           const std::tuple<image, animation, playback, flip, color, transparency> &texture_, const int priority_);
    virtual void on_prepare() {};
    virtual void on_create() {};
    virtual void on_previous() {};
    virtual void on_event(const SDL_Event &) {};
    virtual void on_input(const bool *) {};
    virtual void on_simulate(const double) {};
    virtual void on_collide(const double) {};
    virtual void on_render(const double) {};
    virtual void on_destroy() {};
    virtual void on_clean() {};

  private:
    void prepare();
    void create(SDL_Window *instance, SDL_GPUDevice *gpu);
    void previous();
    void event(const SDL_Event &event);
    void input(const bool *input);
    void simulate(const double poll_rate);
    void collide(const double poll_rate, const name self,
                 const std::unordered_map<name, std::shared_ptr<object>> &objects);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const glm::dmat4 &projection_matrix, const glm::dmat4 &view_matrix, const double alpha);
    void destroy(SDL_GPUDevice *gpu);
    void clean();

  public:
    help::object_state state{};
    help::object_graphics graphics{};
  };
}
