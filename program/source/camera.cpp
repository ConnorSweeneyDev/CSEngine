#include "camera.hpp"

#include "SDL3/SDL_scancode.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

namespace cse::base
{
  camera::transform::property::property(const glm::vec3 &starting_value)
    : current(starting_value), previous(starting_value), interpolated(starting_value),
      velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f))
  {
  }

  camera::transform::transform(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward,
                               const glm::vec3 &starting_up)
    : translation(starting_translation), forward(starting_forward), up(starting_up)
  {
  }

  camera::camera(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up,
                 const float starting_fov, const float starting_near_clip, const float starting_far_clip)
    : graphics(), fov(starting_fov), near_clip(starting_near_clip), far_clip(starting_far_clip),
      transform(starting_translation, starting_forward, starting_up)
  {
  }

  camera::~camera() {}

  void camera::input(const bool *key_state)
  {
    if (key_state[SDL_SCANCODE_I]) transform.translation.acceleration.y += 0.0005f;
    if (key_state[SDL_SCANCODE_K]) transform.translation.acceleration.y -= 0.0005f;
    if (key_state[SDL_SCANCODE_L]) transform.translation.acceleration.x += 0.0005f;
    if (key_state[SDL_SCANCODE_J]) transform.translation.acceleration.x -= 0.0005f;
    if (key_state[SDL_SCANCODE_U]) transform.translation.acceleration.z -= 0.0005f;
    if (key_state[SDL_SCANCODE_O]) transform.translation.acceleration.z += 0.0005f;
  }

  void camera::simulate(double simulation_alpha)
  {
    transform.translation.previous = transform.translation.current;
    transform.translation.velocity += transform.translation.acceleration;
    for (int i = 0; i < 3; ++i)
    {
      transform.translation.acceleration[i] = -0.0001f;
      if (transform.translation.velocity[i] < 0.0f)
        transform.translation.velocity[i] -= transform.translation.acceleration[i];
      if (transform.translation.velocity[i] > 0.0f)
        transform.translation.velocity[i] += transform.translation.acceleration[i];
      if (transform.translation.velocity[i] < 0.0001f && transform.translation.velocity[i] > -0.0001f)
        transform.translation.velocity[i] = 0.0f;
    }
    transform.translation.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.translation.current += transform.translation.velocity;
    transform.translation.interpolated =
      transform.translation.previous +
      ((transform.translation.current - transform.translation.previous) * static_cast<float>(simulation_alpha));
  }

  void camera::render(int width, int height)
  {
    graphics.projection_matrix =
      glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), near_clip, far_clip);
    graphics.view_matrix =
      glm::lookAt(transform.translation.interpolated,
                  transform.translation.interpolated + transform.forward.interpolated, transform.up.interpolated);
  }
}
