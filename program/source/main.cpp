#include <string>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_scancode.h"

#include "utility.hpp"
#include "window.hpp"

struct SDL_AppState
{
  cse::Window window{"CSEngine", 1280, 720};
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  if (argc > 1 || argv[0] == nullptr)
    return cse::utility::log("Expected 1 argument, got " + std::to_string(argc), cse::utility::FAILURE);

  if (!SDL_Init(SDL_INIT_VIDEO)) return cse::utility::log("SDL could not be initialized", cse::utility::SDL_FAILURE);

  SDL_AppState *state = new SDL_AppState();
  if (!state) return cse::utility::log("Application state is nullptr during init", cse::utility::FAILURE);
  *appstate = state;

  if (!state->window.initialized) return cse::utility::log("Window could not be initialized", cse::utility::FAILURE);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state) return cse::utility::log("Application state is nullptr during iterate", cse::utility::FAILURE);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state) return cse::utility::log("Application state is nullptr during event", cse::utility::FAILURE);

  switch (event->type)
  {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MOVED: return state->window.handle_move();
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.scancode)
      {
        case SDL_SCANCODE_ESCAPE: return SDL_APP_SUCCESS;
        case SDL_SCANCODE_F11: return state->window.handle_fullscreen();
        default: break;
      }
    default: break;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (state)
  {
    delete state;
    state = nullptr;
  }
  else
    cse::utility::log("Application state is nullptr during quit", cse::utility::FAILURE);

  cse::utility::log("Application quit", result == SDL_APP_SUCCESS ? cse::utility::SUCCESS : cse::utility::FAILURE);
}
