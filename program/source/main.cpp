#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_video.h"

struct SDL_AppState
{
  enum Direction
  {
    NORTH,
    EAST,
    SOUTH,
    WEST
  };

  SDL_Window *window;
  bool fullscreen = false;
  const int starting_width = 800;
  const int starting_height = 600;
  Direction direction_x = EAST;
  Direction direction_y = SOUTH;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  if (argc > 1 || argv[0] == nullptr) return SDL_APP_FAILURE;

  SDL_AppState *state = new SDL_AppState;
  if (!state) return SDL_APP_FAILURE;
  *appstate = state;

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    delete state;
    return SDL_APP_FAILURE;
  }
  state->window = SDL_CreateWindow("CSEngine", state->starting_width, state->starting_height, 0);
  SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()),
                        SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()));

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);

  int x = 0, y = 0, w = 0, h = 0;
  if (!state->fullscreen)
  {
    SDL_Rect rect = {};
    SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &rect);
    SDL_GetWindowPosition(state->window, &x, &y);
    SDL_GetWindowSize(state->window, &w, &h);

    if (x <= rect.x)
      state->direction_x = SDL_AppState::Direction::EAST;
    else if (x + w >= rect.x + rect.w)
      state->direction_x = SDL_AppState::Direction::WEST;
    if (y <= rect.y)
      state->direction_y = SDL_AppState::Direction::SOUTH;
    else if (y + h >= rect.y + rect.h)
      state->direction_y = SDL_AppState::Direction::NORTH;
    SDL_SetWindowPosition(state->window, (state->direction_x == SDL_AppState::Direction::EAST ? x + 1 : x - 1),
                          (state->direction_y == SDL_AppState::Direction::SOUTH ? y + 1 : y - 1));
  }
  SDL_Log("Window position: %d, %d", x, y);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);

  if (event->type == SDL_EVENT_QUIT)
  {
    int x, y;
    SDL_GetWindowPosition(state->window, &x, &y);
    SDL_Log("Window position: %d, %d", x, y);
    return SDL_APP_SUCCESS;
  }
  else if (event->type == SDL_EVENT_KEY_DOWN)
  {
    if (event->key.scancode == SDL_SCANCODE_ESCAPE)
    {
      int x, y;
      SDL_GetWindowPosition(state->window, &x, &y);
      SDL_Log("Window position: %d, %d", x, y);
      return SDL_APP_SUCCESS;
    }
    if (event->key.scancode == SDL_SCANCODE_F)
    {
      if (state->fullscreen)
      {
        SDL_SetWindowBordered(state->window, true);
        SDL_SetWindowSize(state->window, state->starting_width, state->starting_height);
        SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()),
                              SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()));
        state->fullscreen = false;
      }
      else
      {
        SDL_Rect display_bounds;
        SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &display_bounds);

        SDL_SetWindowBordered(state->window, false);
        SDL_SetWindowSize(state->window, display_bounds.w, display_bounds.h);
        SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()),
                              SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetPrimaryDisplay()));
        state->fullscreen = true;
      }
    }
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  SDL_AppState *state = static_cast<SDL_AppState *>(appstate);
  if (state)
  {
    if (state->window)
    {
      SDL_DestroyWindow(state->window);
      state->window = nullptr;
    }
    delete state;
  }

  SDL_Log("Application quit with result: %s", (result == SDL_APP_SUCCESS)   ? "SUCCESS"
                                              : (result == SDL_APP_FAILURE) ? "FAILURE"
                                                                            : "CONTINUE");
  SDL_Quit();
}
