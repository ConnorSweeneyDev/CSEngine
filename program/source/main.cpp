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
      window.update_delta_time();
      if (window.handle_events() == EXIT_FAILURE) break;
      if (window.update() == EXIT_FAILURE) break;
      window.catchup();
    }
  }

  return cse::utility::log("Application quit", cse::utility::SUCCESS);
}
