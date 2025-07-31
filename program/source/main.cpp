#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include "SDL3/SDL_log.h"
#include "glm/ext/vector_float3.hpp"

#include "camera.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "object.hpp"
#include "resource.hpp"
#include "scene.hpp"
#include "window.hpp"

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::utility::exception("Expected 1 argument, got {}", argc);

  {
    auto window = std::make_shared<cse::base::window>("CSE Example", 1280, 720, false, true);
    auto game = std::make_shared<cse::base::game>(window);
    auto camera = std::make_shared<cse::base::camera>(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                                      glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 0.01f, 10.0f);
    auto default_scene = std::make_shared<cse::base::scene>(camera);
    auto default_quad = std::make_shared<cse::base::object>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                                            glm::vec3(1.0f, 1.0f, 1.0f), cse::resource::main_vertex,
                                                            cse::resource::main_fragment);
    default_scene->add_object("default_quad", default_quad);
    game->add_scene("default_scene", default_scene);
    game->set_current_scene("default_scene");
    game->run();
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
