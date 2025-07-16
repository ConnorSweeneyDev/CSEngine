#include <cstdlib>
#include <string>

#include "utility.hpp"
#include "window.hpp"

int main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0])
    return cse::utility::log("Expected 1 argument, got " + std::to_string(argc), cse::utility::FAILURE);

  {
    cse::Window window{"CSEngine", 1280, 720};
    if (!window.running) return cse::utility::log("Window could not be initialized", cse::utility::FAILURE);
    while (window.running)
    {
      window.update_time();
      while (window.is_behind())
      {
        if (window.input() == EXIT_FAILURE) return cse::utility::log("Input failed", cse::utility::FAILURE);
        window.simulate();
        window.catchup();
      }
      window.update_alpha();
      if (window.render() == EXIT_FAILURE) return cse::utility::log("Render failed", cse::utility::FAILURE);
      window.update_fps();
    }
  }

  return cse::utility::log("Application quit", cse::utility::SUCCESS);
}
