#include <cstdlib>
#include <exception>

#include "SDL3/SDL_log.h"

#include "exception.hpp"
#include "window.hpp"

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::Exception("Expected 1 argument, got {}", argc);

  auto window = cse::Window::create(argv[0], false, 1280, 720);
  while (window->running)
  {
    window->update_simulation_time();
    while (window->simulation_behind())
    {
      window->input();
      window->simulate();
      window->catchup_simulation();
    }
    window->update_simulation_alpha();
    if (window->render_behind())
    {
      window->render();
      window->update_fps();
    }
  }
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
  try
  {
    return try_main(argc, argv);
  }
  catch (const std::exception &exception)
  {
    SDL_Log("%s", exception.what());
    return EXIT_FAILURE;
  }
}
