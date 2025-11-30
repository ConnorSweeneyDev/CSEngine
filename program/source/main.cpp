#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_scancode.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

#include "camera.hpp"
#include "game.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "utility.hpp"
#include "window.hpp"

class custom_scene : public cse::core::scene
{
public:
  custom_scene() {}
};

class custom_window : public cse::core::window
{
public:
  custom_window(const std::string &title, const unsigned int width, const unsigned int height)
    : window(title, width, height, false, true)
  {
    handle_event = [this](const SDL_Event &event)
    {
      if (event.type != SDL_EVENT_KEY_DOWN) return;
      switch (const auto &key = event.key; key.scancode)
      {
        case SDL_SCANCODE_ESCAPE: state.running = false; break;
        case SDL_SCANCODE_F11:
          if (!key.repeat) graphics.fullscreen = !graphics.fullscreen;
          break;
        case SDL_SCANCODE_F12:
          if (!key.repeat) graphics.vsync = !graphics.vsync;
          break;
        default: break;
      }
    };
  }
};

class custom_camera : public cse::core::camera
{
public:
  custom_camera(const glm::vec3 &translation, const glm::vec3 &forward, const glm::vec3 &up)
    : cse::core::camera(translation, forward, up, 45.0f)
  {
    handle_input = [this](const bool *keys)
    {
      if (keys[SDL_SCANCODE_I]) transform.translation.acceleration.y += 0.01f;
      if (keys[SDL_SCANCODE_K]) transform.translation.acceleration.y -= 0.01f;
      if (keys[SDL_SCANCODE_L]) transform.translation.acceleration.x += 0.01f;
      if (keys[SDL_SCANCODE_J]) transform.translation.acceleration.x -= 0.01f;
      if (keys[SDL_SCANCODE_U]) transform.translation.acceleration.z -= 0.01f;
      if (keys[SDL_SCANCODE_O]) transform.translation.acceleration.z += 0.01f;
    };

    handle_simulate = [this]()
    {
      transform.translation.velocity += transform.translation.acceleration;
      transform.translation.acceleration = glm::vec3(-0.002f);
      for (int index = 0; index < 3; ++index)
      {
        if (transform.translation.velocity[index] < 0.0f)
          transform.translation.velocity[index] -= transform.translation.acceleration[index];
        if (transform.translation.velocity[index] > 0.0f)
          transform.translation.velocity[index] += transform.translation.acceleration[index];
        if (transform.translation.velocity[index] < 0.002f && transform.translation.velocity[index] > -0.002f)
          transform.translation.velocity[index] = 0.0f;
      }
      transform.translation.acceleration = glm::vec3(0.0f);
      transform.translation.value += transform.translation.velocity;
    };
  }
};

class custom_object : public cse::core::object
{
public:
  custom_object(const glm::ivec3 &translation, const glm::ivec3 &rotation, const glm::ivec3 &scale)
    : cse::core::object(translation, rotation, scale, cse::resource::main_vertex, cse::resource::main_fragment,
                        cse::resource::main_texture, "main")
  {
    handle_event = [this](const SDL_Event &event)
    {
      if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) return;
      switch (const auto &key = event.key; key.scancode)
      {
        case SDL_SCANCODE_9:
          if (!key.repeat && key.type == SDL_EVENT_KEY_DOWN)
            graphics.texture.frame_group = "other";
          else if (key.type == SDL_EVENT_KEY_UP)
            graphics.texture.frame_group = "main";
          break;
        default: break;
      }
    };

    handle_input = [this](const bool *keys)
    {
      if (keys[SDL_SCANCODE_E]) transform.translation.acceleration.y += 0.01f;
      if (keys[SDL_SCANCODE_D]) transform.translation.acceleration.y -= 0.01f;
      if (keys[SDL_SCANCODE_F]) transform.translation.acceleration.x += 0.01f;
      if (keys[SDL_SCANCODE_S]) transform.translation.acceleration.x -= 0.01f;
      if (keys[SDL_SCANCODE_W]) transform.translation.acceleration.z += 0.01f;
      if (keys[SDL_SCANCODE_R]) transform.translation.acceleration.z -= 0.01f;
    };

    handle_simulate = [this]()
    {
      transform.translation.velocity += transform.translation.acceleration;
      transform.translation.acceleration = glm::vec3(-0.002f);
      for (int index = 0; index < 3; ++index)
      {
        if (transform.translation.velocity[index] < 0.0f)
          transform.translation.velocity[index] -= transform.translation.acceleration[index];
        if (transform.translation.velocity[index] > 0.0f)
          transform.translation.velocity[index] += transform.translation.acceleration[index];
        if (transform.translation.velocity[index] < 0.002f && transform.translation.velocity[index] > -0.002f)
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

  auto game = std::make_unique<cse::core::game>();
  game->set_window<custom_window>("CSE Example", 1280, 720);

  if (auto scene = game->add_scene<custom_scene>("scene").lock())
  {
    scene->set_camera<custom_camera>(glm::vec3(0.0f, 0.0f, 80.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    scene->add_object<custom_object>("object", glm::ivec3(0, 0, 0), glm::ivec3(0, 0, 0), glm::ivec3(1, 1, 1));
  }
  else
    throw cse::utility::exception("Failed to add scene with name '{}'", "scene");

  game->set_current_scene("scene");
  game->run();
  game.reset();

  cse::utility::print("Exiting application...\n");
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
    cse::utility::print_format<cse::utility::CERR>("{}\n", error.what());
    return EXIT_FAILURE;
  }
}
