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
#include "exception.hpp"
#include "game.hpp"
#include "object.hpp"
#include "print.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "system.hpp"
#include "window.hpp"

class custom_window : public cse::core::window
{
public:
  custom_window(const std::string &title_, const unsigned int width_, const unsigned int height_)
    : window(title_, width_, height_, false, true)
  {
    hooks.add("event_main",
              [this](const SDL_Event &event)
              {
                if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) return;
                switch (const auto &key{event.key}; key.scancode)
                {
                  case SDL_SCANCODE_ESCAPE: running = false; break;
                  case SDL_SCANCODE_F11: fullscreen = !fullscreen; break;
                  case SDL_SCANCODE_F12: vsync = !vsync; break;
                  default: break;
                }
              });
  }
};

class custom_scene : public cse::core::scene
{
public:
  custom_scene() : scene() {}
};

class custom_camera : public cse::core::camera
{
public:
  custom_camera(const glm::vec3 &translation_, const glm::vec3 &forward_, const glm::vec3 &up_)
    : camera(translation_, forward_, up_, 45.0f)
  {
    hooks.add("input_main",
              [this](const bool *keys)
              {
                if (keys[SDL_SCANCODE_I]) state.translation.acceleration.y += 0.01f;
                if (keys[SDL_SCANCODE_K]) state.translation.acceleration.y -= 0.01f;
                if (keys[SDL_SCANCODE_L]) state.translation.acceleration.x += 0.01f;
                if (keys[SDL_SCANCODE_J]) state.translation.acceleration.x -= 0.01f;
                if (keys[SDL_SCANCODE_U]) state.translation.acceleration.z -= 0.01f;
                if (keys[SDL_SCANCODE_O]) state.translation.acceleration.z += 0.01f;
              });

    hooks.add("simulate_main",
              [this]()
              {
                state.translation.velocity += state.translation.acceleration;
                state.translation.acceleration = glm::vec3(-0.002f);
                for (int index{}; index < 3; ++index)
                {
                  if (state.translation.velocity[index] < 0.0f)
                    state.translation.velocity[index] -= state.translation.acceleration[index];
                  if (state.translation.velocity[index] > 0.0f)
                    state.translation.velocity[index] += state.translation.acceleration[index];
                  if (state.translation.velocity[index] < 0.002f && state.translation.velocity[index] > -0.002f)
                    state.translation.velocity[index] = 0.0f;
                }
                state.translation.acceleration = glm::vec3(0.0f);
                state.translation.value += state.translation.velocity;
              });
  }
};

class custom_object : public cse::core::object
{
public:
  custom_object(const glm::ivec3 &translation_, const glm::ivec3 &rotation_, const glm::ivec3 &scale_)
    : object(translation_, rotation_, scale_, {255, 255, 255, 255}, cse::resource::main_vertex,
             cse::resource::main_fragment, cse::resource::main_texture, "main")
  {
    hooks.add("event_main",
              [this](const SDL_Event &event)
              {
                if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) return;
                switch (const auto &key{event.key}; key.scancode)
                {
                  case SDL_SCANCODE_0:
                    if (!key.repeat && key.type == SDL_EVENT_KEY_DOWN)
                      graphics.texture.frame_group = "other";
                    else if (key.type == SDL_EVENT_KEY_UP)
                      graphics.texture.frame_group = "main";
                    break;
                  default: break;
                }
              });

    hooks.add("input_main",
              [this](const bool *keys)
              {
                if (keys[SDL_SCANCODE_E]) state.translation.acceleration.y += 0.01f;
                if (keys[SDL_SCANCODE_D]) state.translation.acceleration.y -= 0.01f;
                if (keys[SDL_SCANCODE_F]) state.translation.acceleration.x += 0.01f;
                if (keys[SDL_SCANCODE_S]) state.translation.acceleration.x -= 0.01f;
                if (keys[SDL_SCANCODE_W]) state.translation.acceleration.z += 0.01f;
                if (keys[SDL_SCANCODE_R]) state.translation.acceleration.z -= 0.01f;
              });

    hooks.add("simulate_main",
              [this]()
              {
                state.translation.velocity += state.translation.acceleration;
                state.translation.acceleration = glm::vec3(-0.002f);
                for (int index{}; index < 3; ++index)
                {
                  if (state.translation.velocity[index] < 0.0f)
                    state.translation.velocity[index] -= state.translation.acceleration[index];
                  if (state.translation.velocity[index] > 0.0f)
                    state.translation.velocity[index] += state.translation.acceleration[index];
                  if (state.translation.velocity[index] < 0.002f && state.translation.velocity[index] > -0.002f)
                    state.translation.velocity[index] = 0.0f;
                }
                state.translation.acceleration = glm::vec3(0.0f);
                state.translation.value += state.translation.velocity;
              });
  }
};

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::utility::exception("Expected 1 argument, got {}", argc);

  auto game{std::make_unique<cse::core::game>()};
  game->set_window<custom_window>("CSE Example", 1280, 720);

  if (auto scene{game->add_scene<custom_scene>("scene").lock()})
  {
    scene->set_camera<custom_camera>(glm::vec3(0.0f, 0.0f, 80.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
    scene->add_object<custom_object>("object", glm::ivec3(0, 0, 0), glm::ivec3(0, 0, 0), glm::ivec3(1, 1, 1));
  }
  game->set_current_scene("scene");

  game->run();
  game.reset();

  if constexpr (cse::system::debug) cse::utility::print<COUT>("Exiting application...\n");
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
    cse::utility::print<CERR>("{}\n", error.what());
    return EXIT_FAILURE;
  }
}
