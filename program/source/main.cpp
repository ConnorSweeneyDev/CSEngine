#include "main.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_filesystem.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_stdinc.h"
#undef main

#include "core.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "log.hpp"
#include "meta.hpp"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  try
  {
    const std::span<char *> args(argv, static_cast<std::size_t>(argc));
    auto application = cse::main(std::vector<std::string_view>(args.begin(), args.end()));

    if (!application.instance) throw cse::exception("Game instance is null before initialization");
    *appstate = application.instance.get();

    cse::meta = {application.meta.organization, application.meta.application, application.meta.version};
    if (cse::meta.organization.empty() || cse::meta.application.empty() || cse::meta.version.empty())
      throw cse::exception("Application metadata must be set");
    char *path{SDL_GetPrefPath(cse::meta.organization.c_str(), cse::meta.application.c_str())};
    if (!path)
      cse::sdl_log("Failed to get preferred system path");
    else
      cse::meta.output = path;
    SDL_free(path);

    return application.instance->initialize();
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
