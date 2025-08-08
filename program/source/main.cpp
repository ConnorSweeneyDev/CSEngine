#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

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
  custom_window(const std::string &title_, int starting_width_, int starting_height_)
    : window(title_, starting_width_, starting_height_, false, true)
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
  custom_camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : cse::base::camera(translation_, forward_, up_, 45.0f, 0.1f, 10.0f)
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

    handle_simulate = [this]()
    {
      transform.translation.velocity += transform.translation.acceleration;
      transform.translation.acceleration = glm::vec3(-0.0001f);
      for (int index = 0; index < 3; ++index)
      {
        if (transform.translation.velocity[index] < 0.0f)
          transform.translation.velocity[index] -= transform.translation.acceleration[index];
        if (transform.translation.velocity[index] > 0.0f)
          transform.translation.velocity[index] += transform.translation.acceleration[index];
        if (transform.translation.velocity[index] < 0.0001f && transform.translation.velocity[index] > -0.0001f)
          transform.translation.velocity[index] = 0.0f;
      }
      transform.translation.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
      transform.translation.value += transform.translation.velocity;
    };
  }
};

class custom_object : public cse::base::object
{
public:
  custom_object(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_)
    : cse::base::object(translation_, rotation_, scale_, cse::resource::main_vertex, cse::resource::main_fragment,
                        cse::resource::main_texture, 1)
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

    handle_simulate = [this]()
    {
      transform.translation.velocity += transform.translation.acceleration;
      transform.translation.acceleration = glm::vec3(-0.0001f);
      for (int index = 0; index < 3; ++index)
      {
        if (transform.translation.velocity[index] < 0.0f)
          transform.translation.velocity[index] -= transform.translation.acceleration[index];
        if (transform.translation.velocity[index] > 0.0f)
          transform.translation.velocity[index] += transform.translation.acceleration[index];
        if (transform.translation.velocity[index] < 0.0001f && transform.translation.velocity[index] > -0.0001f)
          transform.translation.velocity[index] = 0.0f;
      }
      transform.translation.acceleration = glm::vec3(0.0f);
      transform.translation.value += transform.translation.velocity;
    };
  }
};

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::utility::exception("Expected 1 argument, got {}", argc);

  auto game = std::make_unique<cse::game>();
  game->set_window<custom_window>("CSE Example", 1280, 720);

  game->add_scene<cse::base::scene>("scene");
  if (auto scene = game->get_scene("scene").lock())
  {
    scene->set_camera<custom_camera>(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    scene->add_object<custom_object>("object", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(1.0f, 1.0f, 1.0f));
  }
  else
    throw cse::utility::exception("Failed to get scene with name '{}'", "scene");

  game->set_current_scene("scene");
  game->run();
  game.reset();

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
