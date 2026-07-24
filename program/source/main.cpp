#include "main.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#undef main

#include "exception.hpp"
#include "game.hpp"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  try
  {
    const std::span<char *> arguments(argv, static_cast<std::size_t>(argc));
    auto instance = cse::main(std::vector<std::string_view>(arguments.begin(), arguments.end()));
    if (!instance) throw cse::exception("Game instance is null before initialization");
    *appstate = instance.get();
    return instance->initialize();
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  try
  {
    auto instance{static_cast<cse::game *>(appstate)};
    if (!instance) throw cse::exception("Game instance is null before receiving events");
    return instance->receive(*event);
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  try
  {
    auto instance{static_cast<cse::game *>(appstate)};
    if (!instance) throw cse::exception("Game instance is null before iteration");
    return instance->iterate();
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
    return SDL_APP_FAILURE;
  }
}

void SDL_AppQuit(void *appstate, SDL_AppResult)
{
  try
  {
    auto instance{static_cast<cse::game *>(appstate)};
    if (!instance) return;
    instance->quit();
  }
  catch (const std::exception &error)
  {
    cse::exception::report(error);
  }
}
