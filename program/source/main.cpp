#include <cstdlib>
#include <exception>
#include <memory>

#include "SDL3/SDL_log.h"

#include "exception.hpp"
#include "window.hpp"

int try_main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw cse::exception("Expected 1 argument, got {}", argc);

  std::shared_ptr<cse::window> window = cse::window::create(argv[0], 1280, 720, false, true);
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
  window.reset();

  SDL_Log("Exiting application...");
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
