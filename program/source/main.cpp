#include <cstdlib>
#include <memory>
#include <string>

#include "utility.hpp"
#include "window.hpp"

int main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0])
    return cse::utility::log("Expected 1 argument, got " + std::to_string(argc), cse::utility::FAILURE);

  if (auto window = cse::Window::create(argv[0], false, 1280, 720); !cse::Window::valid(window))
    return cse::utility::log("Window is invalid", cse::utility::FAILURE);
  else
    while (window->running)
    {
      window->update_simulation_time();
      while (window->simulation_behind())
      {
        if (window->input() == EXIT_FAILURE) return cse::utility::log("Input failed", cse::utility::FAILURE);
        window->simulate();
        window->catchup_simulation();
      }
      window->update_simulation_alpha();
      if (window->render_behind())
      {
        if (window->render() == EXIT_FAILURE) return cse::utility::log("Render failed", cse::utility::FAILURE);
        window->update_fps();
      }
    }

  return cse::utility::log("Application quit", cse::utility::SUCCESS);
}
