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
  if (argc > 1 || argv[0] == nullptr) return cse::utility::log("Expected 1 argument", SDL_APP_FAILURE, true);

  if (!SDL_Init(SDL_INIT_VIDEO)) return cse::utility::log("SDL could not be initialized", SDL_APP_FAILURE, true);

  SDL_AppState *state = new SDL_AppState();
  if (!state) return cse::utility::log("Memory for application state could not be allocated", SDL_APP_FAILURE, true);
  *appstate = state;

  if (!state->window.initialized) return cse::utility::log("Window could not be initialized", SDL_APP_FAILURE, true);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state) return cse::utility::log("Application state is nullptr during iterate", SDL_APP_FAILURE);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state) return cse::utility::log("Application state is nullptr during event", SDL_APP_FAILURE);

  switch (event->type)
  {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MOVED:
      if (!state->window.handle_move()) return cse::utility::log("Could not handle window move", SDL_APP_FAILURE, true);
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.scancode)
      {
        case SDL_SCANCODE_ESCAPE: return SDL_APP_SUCCESS;
        case SDL_SCANCODE_F11:
          if (!state->window.handle_fullscreen())
            return cse::utility::log("Could not handle fullscreen", SDL_APP_FAILURE, true);
          break;
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
    cse::utility::log("Application state is nullptr during quit", SDL_APP_FAILURE);

  cse::utility::log("Application quit", result);
}
