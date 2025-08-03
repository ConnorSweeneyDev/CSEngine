#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_scancode.h"
#include "glm/ext/vector_float3.hpp"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "window.hpp"

class custom_window : public cse::base::window
{
public:
  custom_window(const std::string &i_title, int i_starting_width, int i_starting_height, bool i_fullscreen,
                bool i_vsync)
    : window(i_title, i_starting_width, i_starting_height, i_fullscreen, i_vsync)
  {
    handle_input = [this](const SDL_KeyboardEvent &key)
    {
      switch (key.scancode)
      {
        case SDL_SCANCODE_ESCAPE: quit(); break;
        case SDL_SCANCODE_F11: toggle_fullscreen(); break;
        case SDL_SCANCODE_F12: toggle_vsync(); break;
        default: break;
      }
    };
  }
};

class custom_camera : public cse::base::camera
{
public:
  custom_camera(const glm::vec3 &starting_translation, const glm::vec3 &starting_forward, const glm::vec3 &starting_up,
                float starting_fov, float starting_near_clip, float starting_far_clip)
    : cse::base::camera(starting_translation, starting_forward, starting_up, starting_fov, starting_near_clip,
                        starting_far_clip)
  {
    handle_input = [this](const bool *key_state)
    {
      if (key_state[SDL_SCANCODE_I]) transform.translation.acceleration.y += 0.0005f;
      if (key_state[SDL_SCANCODE_K]) transform.translation.acceleration.y -= 0.0005f;
      if (key_state[SDL_SCANCODE_L]) transform.translation.acceleration.x += 0.0005f;
      if (key_state[SDL_SCANCODE_J]) transform.translation.acceleration.x -= 0.0005f;
      if (key_state[SDL_SCANCODE_U]) transform.translation.acceleration.z -= 0.0005f;
      if (key_state[SDL_SCANCODE_O]) transform.translation.acceleration.z += 0.0005f;
    };

    handle_simulate = [this](double simulation_alpha)
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
    };
  }
};

class custom_object : public cse::base::object
{
public:
  custom_object(const glm::vec3 &starting_translation, const glm::vec3 &starting_rotation,
                const glm::vec3 &starting_scale, const cse::resource::compiled_shader &vertex_shader,
                const cse::resource::compiled_shader &fragment_shader)
    : cse::base::object(starting_translation, starting_rotation, starting_scale, vertex_shader, fragment_shader)
  {
    handle_input = [this](const bool *key_state)
    {
      if (key_state[SDL_SCANCODE_E]) transform.translation.acceleration.y += 0.0005f;
      if (key_state[SDL_SCANCODE_D]) transform.translation.acceleration.y -= 0.0005f;
      if (key_state[SDL_SCANCODE_F]) transform.translation.acceleration.x += 0.0005f;
      if (key_state[SDL_SCANCODE_S]) transform.translation.acceleration.x -= 0.0005f;
      if (key_state[SDL_SCANCODE_W]) transform.translation.acceleration.z += 0.0005f;
      if (key_state[SDL_SCANCODE_R]) transform.translation.acceleration.z -= 0.0005f;
    };

    handle_simulate = [this](double simulation_alpha)
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
    };
  }
};

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::utility::exception("Expected 1 argument, got {}", argc);

  {
    auto window = std::make_unique<custom_window>("CSE Example", 1280, 720, false, true);
    auto game = cse::base::game(std::move(window));
    auto camera = std::make_unique<custom_camera>(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                                  glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 0.01f, 10.0f);
    auto scene = std::make_unique<cse::base::scene>(std::move(camera));
    auto quad = std::make_unique<custom_object>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                                glm::vec3(1.0f, 1.0f, 1.0f), cse::resource::main_vertex,
                                                cse::resource::main_fragment);
    scene->add_object("quad", std::move(quad));
    game.add_scene("scene", std::move(scene));
    game.set_current_scene("scene");
    game.run();
  }

  SDL_Log("Exiting application...");
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
  try
  {
    return try_main(argc, argv);
  }
  catch (const std::exception &error)
  {
    SDL_Log("%s", error.what());
    return EXIT_FAILURE;
  }
}
