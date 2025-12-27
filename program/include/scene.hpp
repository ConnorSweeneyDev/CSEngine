#pragma once

#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "camera.hpp"
#include "declaration.hpp"
#include "hooks.hpp"
#include "id.hpp"
#include "object.hpp"

namespace cse
{
  class scene : public std::enable_shared_from_this<scene>
  {
    friend class game;

  public:
    scene() = default;
    virtual ~scene();
    scene(const scene &) = delete;
    scene &operator=(const scene &) = delete;
    scene(scene &&) = delete;
    scene &operator=(scene &&) = delete;

    template <typename camera_type, typename... camera_arguments>
    void set_camera(const std::tuple<glm::vec3, glm::vec3, glm::vec3> &transform, camera_arguments &&...arguments);
    template <typename object_type, typename... object_arguments>
    void set_object(const help::id name, const std::tuple<glm::ivec3, glm::ivec3, glm::ivec3> &transform,
                    object_arguments &&...arguments);
    bool remove_object(const help::id name);

  private:
    void initialize(SDL_Window *instance, SDL_GPUDevice *gpu);
    void event(const SDL_Event &event);
    void input(const bool *keys);
    void simulate(const double simulation_alpha);
    void render(SDL_GPUDevice *gpu, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                const float target_aspect_ratio, const float global_scale_factor);
    void cleanup(SDL_GPUDevice *gpu);

    void process_removals();

  public:
    std::weak_ptr<class game> parent{};
    std::shared_ptr<class camera> camera{};
    std::unordered_map<help::id, std::shared_ptr<object>> objects{};
    help::hooks hooks{};

  private:
    bool initialized{};
    std::set<help::id> pending_removals{};
  };
}

#include "scene.inl" // IWYU pragma: keep
