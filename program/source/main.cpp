#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_scancode.h"

#include "window.hpp"

struct SDL_AppState
{
  cse::Window window{"CSEngine", 1280, 720};
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  SDL_AppState *state = new SDL_AppState();
  if (!state)
  {
    SDL_Log("Failed to allocate memory for application state");
    return SDL_APP_FAILURE;
  }
  *appstate = state;

  if (argc > 1 || argv[0] == nullptr) return SDL_APP_FAILURE;

  if (!state->window.initialized)
  {
    SDL_Log("Failed to initialize window");
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state)
  {
    SDL_Log("Application state is nullptr");
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (!state)
  {
    SDL_Log("Application state is nullptr");
    return SDL_APP_FAILURE;
  }

  switch (event->type)
  {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MOVED:
      if (!state->window.handle_move())
      {
        SDL_Log("Failed to handle window move");
        return SDL_APP_FAILURE;
      }
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.scancode)
      {
        case SDL_SCANCODE_ESCAPE: return SDL_APP_SUCCESS;
        case SDL_SCANCODE_F:
          if (!state->window.handle_fullscreen())
          {
            SDL_Log("Failed to handle fullscreen");
            return SDL_APP_FAILURE;
          }
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
    SDL_Log("Application state is nullptr");

  SDL_Log("Application quit with result: %s", (result == SDL_APP_SUCCESS)   ? "SUCCESS"
                                              : (result == SDL_APP_FAILURE) ? "FAILURE"
                                                                            : "CONTINUE");
}
