<div align="center">

# CSEngine

**A game engine for creating 2D/3D hybrid pixel art games with ease.**

<video src="https://github.com/user-attachments/assets/4aec7a39-614f-448f-8a55-f4d76f9b3eb5"></video>
<em>A video of a dimly lit scene with text, audio, a custom cursor, an animated sprite and soft shadows.</em>

</div>

## Requirements
- Windows or Linux OS.
- A C++20 compiler.

## Usage
CSEngine is a static library (`cse`) that you link into your own executable. You describe your game declaratively as a
tree of named entities, override lifecycle hooks to add behaviour, and the engine drives a fixed-timestep simulation
with an interpolated render loop on top of SDL3 (GPU and Mixer).

The canonical, complete example of everything below is the [CSGame](https://github.com/ConnorSweeneyDev/CSGame) project;
it is the reference consumer and a good template to copy.

### Mental Model
- **A `game`** owns a persistent `state`, a `window`, a current `scene`, and a variable amount of `scenes` and
  `interfaces`.
- **A `scene`** owns one `camera`, and a variable amount of `interfaces`, `objects` and `lights`.
- **An `interface`** is a 2D overlay (HUD, menu, etc.), whereas a `camera`/`object`/`light` is a 3D entity in the scene.
- **Entities are added by name** (`"player"`, `"button1"`, …) and looked up by name later.
- **You never write a `main` loop.** You subclass the engine types, override `on_*`/`pre_*`/`post_*` hooks, and the
  engine calls them in the right order each tick/frame.
- **Simulation is fixed-timestep** (e.g. 300Hz); **rendering is decoupled and interpolated** (e.g. 144fps). Values that
  change over time are declared as `temporal<T>` so the renderer can smoothly interpolate between the previous and
  current state, and to make it easier to drive them without extra state.
- **Everything has an `active` and previous snapshot**, so behaviour can compare "this tick vs last tick" (e.g.
  detecting a hover that just started).

### Project Layout
It is highly recommended to use CSBuild as your build system. An example project layout could look like this:

```
game/
| csb/
| | script/
| | csb.hpp
| | csb.cpp
| program/
| | include/*.[hpp|inl] # headers
| | source/*.cpp        # sources
| | texture/*.aseprite  # sprite sheets (with animation/hitbox metadata)
| | font/*.aseprite     # bitmap fonts (glyph atlases with slice metadata)
| | sound/*.wav         # short sound effects
| | music/*.opus        # streamed music
```

There are very specific rules about the format of the aseprite, wav and opus files, all of which are detailed in the
[CSData](https://github.com/ConnorSweeneyDev/CSData) helper repository. Ensure that your assets are valid, any invalid
ones error out at build time so you can find them.

### Build System Jobs
After pulling in CSEngine, it builds with [CSBuild](https://github.com/ConnorSweeneyDev/CSBuild) (run
`csb\script\build.bat` on Windows or `./csb/script/build.sh` on Linux from the project directory). Your build system
should do the following:

1. **Includes and Library Paths.** All library headers will be copied into `build/include`, and all libraries will be
   copied into `build/[release|debug]`. It is your job to include the headers and link the libraries.
2. **Required Compilation Flags.** On MSVC, compile with at least `/std:c++20`, and `/bigobj`, `/Zc:preprocessor`.
3. **Packing & Embedding Assets.** `.aseprite` textures and fonts, `.wav` and `.opus` audio are packed into `.csp`
   containers (via [CSPack](https://github.com/ConnorSweeneyDev/CSPack)) and a `resource.hpp`/`resource.cpp` pair is
   generated that exposes every asset as a typed C++ symbol in your namespace. Shaders are engine-owned. In csb this can
   be done like so:
   ```cpp
   csb::subproject_install({"ConnorSweeneyDev/CSEngine", "1.0.0", COMPILED_LIBRARY});
   csb::pack(csb::choose_files({"program/texture"}), csb::choose_files({"program/font"}),
             csb::choose_files({"program/sound"}), csb::choose_files({"program/music"}),
             [](const std::filesystem::path &file) -> std::string
             {
               const auto parent{file.parent_path().filename().string()};
               if (parent == "texture" || parent == "font" || parent == "sound" || parent == "music") return "CSGame";
               return parent;
             },
             "csg", {"program/include/resource.hpp", "program/source/resource.cpp"});
   ```

### Entry Point
Define `cse::main` - the engine provides the real entry point (SDL's app callbacks) and wraps your code in error
handling. Create the game from a setup function and return it; the engine then drives it through `SDL_AppInit`,
`SDL_AppEvent`, `SDL_AppIterate` and `SDL_AppQuit`:

```cpp
#include "cse/main.hpp"
#include "cse/game.hpp"

#include "game.hpp"

std::shared_ptr<cse::game> cse::main(const std::vector<std::string_view> &arguments)
{
  if (arguments.size() != 1) throw exception("Expected 1 argument, got {}", arguments.size());
  return game::create(custom::game::setup);
}
```

### Composing the Game
Subclass `cse::game` and set tuning defaults in the constructor:

```cpp
namespace custom
{
  class game final : public cse::game
  {
  public:
    game();
    static void setup(const std::shared_ptr<game> &game);

  protected:
    void pre_create() final;
    void pre_event(const SDL_Event &event) final;
    void pre_simulate(const double tick) final;
    void post_destroy() final;
  };

  game::game()
    : cse::game({.meta = {.organization = "ConnorSweeneyDev", .application = "CSGame", .version = "1.0.0"},
                 .tick = 300.0,
                 .frame = 144.0,
                 .aspect = {.ratio = 16.0 / 9.0, .resolution = 180, .scaling = VIRTUAL},
                 .clear = {{0.0, 0.0, 0.0}},
                 .memory = {.vram = 512, .ram = 128},
                 .language = language::EN,
                 .master = {0.5},
                 .sound = {1.0},
                 .music = {1.0}}) {};
}
```

`meta` is used to generate the user-data dirctory path (the version does not affect the path) and application metadata.

`tick` and `frame` are the simulation and render rates respectively. The tick rate will always be maintained as long as
the game is not too demanding; if the tick rate ever falls below the target, the game will still be deterministic, but
will run slower than real time. The frame rate is a target; this means that even if the game can easily render at your
target frame rate, the user's system may be configured to limit it to a lower value, so you should not rely on it.

`aspect` bundles the three things that define the virtual canvas. `ratio` is a `double` that defines the aspect ratio
the game will adhere to no matter the size of the window. `resolution` is the canvas **height** in virtual pixels; the
width is derived as `resolution × ratio`. `scaling` chooses how the canvas is fitted to the window:
- `VIRTUAL` (default) scales by whole multiples only, so every virtual pixel is an exact block of device pixels. The
  remainder is letterboxed. Crisp at any window size, at the cost of black bars when the window is not a clean multiple.
- `PHYSICAL` fits the canvas to the window and letterboxes only for aspect. Uses the whole screen, but virtual pixels
  land on fractional device pixels, so sprite edges and glyph stems shimmer slightly during motion.
Pick `resolution` so it divides your target display heights, and is a multiple of your aspect ratio height: at 16:9,
**180** (320×180) scales exactly 2x/4×/6×/8×/12× at 360p/720p/1080p/1440p/2160p.

`clear` is the background colour for the canvas. This affects the clear colour for the 3D scene, and the colour of the
letterbox bars when the canvas does not fill the window.

`memory` is the maximum amount of RAM and VRAM (in MB) the engine will allocate for assets. Unused assets (oldest first)
are automatically evicted when the limit is reached. If so many assets are loaded that the limit cannot be respected,
thrashing will occur which could cause performance issues.

`language` selects which set of translations text resolves against, and is empty by default. It is only meaningful if
you declare languages with the `LANGUAGES` macro; if you do declare them, leaving it empty or naming a language that was
never declared logs a warning and falls back to the first language you declared, overwriting the value.

`master`, `sound` and `music` are the global volume buses for all audio. Each is a `temporal<double>` in the range
[0.0, 1.0] that multiplies every track's own `volume` temporal.

The `setup` function builds the whole entity tree. `set<...>` registers an entity; `current` registers a scene *and*
makes it the active one, but calling it without a config function just switches to an already-registered scene:

```cpp
void game::setup(const std::shared_ptr<game> &g)
{
  g->set<settings>("settings");
  g->set<window>();
  g->current("main", scene::main);
  g->set<cursor>("cursor");
}
```

A scene's contents are populated by a config function passed to `set`/`current`. Note `set<camera>` takes no name (a
scene has exactly one), while objects, lights and interfaces are named:

```cpp
void scene::main(const std::shared_ptr<scene> &s)
{
  s->set<button>("button1", glm::dvec2{-22.0, 40.0});
  s->set<camera>(glm::dvec3{0.0, 0.0, 80.0});
  s->set<player>("player", glm::dvec3{0.0, -6.0, 0.0});
  s->set<environment>("floor", glm::dvec3{0.0, -61.0, 0.0}, image::floor, animation::floor.main);
  s->set<sun>("sun");
}
```

You can swap scenes at runtime with `game->current("name")` (re-use a registered scene) or `game->current("name",
config)` (build it on the fly), and add/remove entities live with `scene->set<...>(...)` / `scene->remove("name")`.

### Defining Entities
Every entity type follows the same shape: subclass the engine base, pass an `initial` struct (designated initializers)
to the base constructor, and override the hooks you care about. An object with every single field configured could look
like this:

```cpp
class player final : public cse::object
{
public:
  player(const glm::dvec3 &translation_);

protected:
  void on_event(const SDL_Event &event) final;
  void on_simulate(const double tick) final;
};

player::player(const glm::dvec3 &translation_)
  : cse::object({.translation = {translation_},
                 .rotation = {0.0},
                 .scale = {{1.0, 1.0}},
                 .collidable = true,
                 .texture = {.source = {.image = image::redhood, .animation = animation::redhood.idle},
                             .playback = {.frame = 0, .elapsed = 0.0, .playing = true, .speed = {1.0}, .loop = true},
                             .flip = {.horizontal = false, .vertical = false},
                             .color = {.tint = {{0.5, 0.5, 0.5, 1.0}}, .alpha = {1.0}},
                             .illumination = {.show = true, .brightness = {1.0}, .penetration = {1.0}},
                             .shadow = {.show = true, .cast = true, .darkness = {1.0}, .softness = {1.0}}},
                 .text = {.content = {"[", lexeme::player, "]"},
                          .source = {.font = font::text, .animation = animation::text.main},
                          .playback = {.frame = 0, .elapsed = 0.0, .playing = false, .speed = {0.0}, .loop = false},
                          .align = {.horizontal = {.preset = CENTER, .spacing = {0.0}},
                                    .vertical = {.preset = TOP, .spacing = {0.0}},
                                    .offset = {{0.0, -5.0}}},
                          .scale = {{1.0, 1.0}},
                          .overflow = {.wrap = false, .clip = false},
                          .color = {.tint = {{0.5, 0.5, 0.5, 1.0}}, .alpha = {1.0}},
                          .illumination = {.show = true, .brightness = {1.0}, .penetration = {1.0}},
                          .shadow = {.show = false, .cast = true, .darkness = {1.0}, .softness = {0.5}}},
                 .priority = {.simulation = 0, .rendering = 1}}) {};
```

All the core classes are defined the same way against their own `initial` structs.

### Lifecycle Hooks
Hooks are virtual no-ops you override. **Leaf entities** (`window`, `interface`, `camera`, `object`, `light`) expose
`on_*` hooks; **`game` and `scene`** expose `pre_*`/`post_*` hooks that bracket the work of their children.

The best way to describe the lifecycle of an entity is to show you the game's SDL app callbacks:

```cpp
SDL_AppResult game::initialize()
{
  if (active.phase == help::phase::CLEANED) prepare();
  if (active.phase == help::phase::PREPARED)
  {
    create();
    synchronize();
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult game::receive(const SDL_Event &event)
{
  if (active.phase != help::phase::CREATED) throw exception("Game must be created before receiving events");
  events.push_back(event);
  return running() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

SDL_AppResult game::iterate()
{
  if (active.phase != help::phase::CREATED) throw exception("Game must be created before iteration");
  step();
  while (behind())
  {
    tps();
    synchronize();
    event();
    simulate();
    collide();
    tps();
  }
  if (ready())
  {
    fps();
    render();
    mix();
    fps();
  }
  return running() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

void game::quit()
{
  if (active.phase == help::phase::CREATED) destroy();
  if (active.phase == help::phase::PREPARED) clean();
  instance.reset();
}
```

For the most part, every entity follows this lifecycle - the only difference being that any of the core classes other
than game don't necessarily call (prepare and create)/(destroy and clean) one after another - prepare is called at the
instantiation of the entity in memory, and create is called when the entity becomes active. A similar behaviour
describes destroy and clean.

### The Temporal Value Model
Any value that should animate smoothly is a `temporal<T>`. It carries a `value`, an optional `rate` (first derivative)
and `curve` (used for acceleration), and an `interpolate` flag the renderer uses to blend between ticks. Drive physics
by writing the value/rate inside `on_simulate`:

```cpp
void player::on_simulate(const double tick)
{
  const auto &keyboard{scene->game->active.window->active.keyboard};
  auto &position{active.translation.value};
  auto &velocity{active.translation.rate};
  auto &acceleration{active.translation.curve};
  if (keyboard[SDL_SCANCODE_E]) acceleration.y += max_velocity;
  if (keyboard[SDL_SCANCODE_D]) acceleration.y -= max_velocity;
  velocity += acceleration * tick;
  acceleration = {0.0, 0.0, 0.0};
  position += velocity * tick;
}
```

Set `.instant = true` on a temporal when you want a hard cut (no interpolation) for one frame.

### Accessing Runtime State
- `active.*` is the current snapshot; `previous.*` is last tick's - compare them to detect transitions.
- Reach related entities through pointers: every entity has `game`, and scene-owned entities also have `scene`. So
  `scene->game->active.window->active.keyboard`, or from a game-level interface, `game->active.window->active.mouse`.
- Look entities up by name with the custom container class the engine uses, and make use of the type system with the
  helpers it provides, or the standalone versions in `cse/pointer.hpp`:
  ```cpp
  active.interfaces["tick"]->active.text.content = ...;               // throws if absent
  auto player = active.objects.find("player");                        // nullptr if absent
  auto settings = as<custom::settings>(active.states["settings"]);    // throw if absent + throw if mismatch
  settings = try_as<custom::settings>(active.states.find("settings"); // nullptr if absent + nullptr if mismatch
  if (is<player>(contact.target.pointer)) { ... }                     // throws if nullptr
  ```

### Starting and Calling Timers
Schedule one-shot or repeating callbacks on any entity's `active.timer`. `set` returns the timer's modifiable `state`.
`call` returns whether it fired (discards any callback return). `capture` is for non-void callbacks and returns
`optional<return_type>`:

```cpp
// Set a timer for 0.5 seconds that clears the text content when it fires
auto &hide = active.timer.set("hide_text", [this]() { active.text.content = {}; });
hide.target = 0.5;
if (active.timer.call<void()>("hide_text")) { /* fired */ }

// Set a timer for every 1 second that repeats indefinitely
auto &tick = active.timer.set("heartbeat", [this]() { /* ... */ });
tick.target = 1.0;
tick.repeat = true;

// Use the return value of a timer callback
if (auto result = active.timer.capture<int()>("roll"))
  use(*result);
```

### Managing Audio
Each game, scene and entity has a `mixer`. Set tracks by name, then toggle/seek/pitch:

```cpp
auto &song = active.mixer.set("main", music::main);
song.loop = true;
song.speed.value = 0.80;
song.playing = true;

active.mixer.set({{"sample1", sound::sample1}, {"sample2", sound::sample2}});
auto &sfx = active.mixer.get<cse::sound>("sample1");
sfx.elapsed = {};
sfx.playing = true;
active.mixer.remove<cse::music>("main");
```

Removing is optional; the mixer will automatically clear tracks when the entity is destroyed.\
The game's `master`, `sound` and `music` temporals act as global buses for volume, all affecting each track's own
`volume` temporal.\

A track's `elapsed` is a `cse::elapsed`, which carries one clock per owner - and which one you read matters:

```cpp
sfx.elapsed.device; // seconds - owned by the audio device, real time
sfx.elapsed.tick;   // seconds - owned by the simulation, tick time
```

`elapsed.device` is written by the device every time the game renders, so it always runs in real time and is only
sampled on frames that actually render. Under heavy lag the simulation falls behind real time and renders are starved
first, so it advances faster than the game does and in irregular jumps. It is for presentation - **never drive gameplay
from it.** Writing it seeks the device.

`elapsed.tick` is advanced in `simulate` against the track's baked `source.duration`, so it ticks at the same rate as
everything else in the simulation and is reproducible no matter how badly the game lags. It stops dead at `duration` on
a non-looping track and wraps on a looping one, so a track has run out exactly when:

```cpp
const auto &sfx = active.mixer.get<cse::sound>("sample1");
if (!sfx.loop && sfx.elapsed.tick >= sfx.source.duration) do_something();
```

### Persistent State
A `state` is a JSON-backed settings blob saved under the OS user-data directory. Declare fields with the `ENLIST` macro
(it builds a plain struct *and* its JSON serializers in one place - pass each field as a `(name, type, init)` tuple, add
as many as you like), expose them with `STORE`, and call `read()`/`write()` which return `false` if anything unexpected
happens (missing file does not count as unexpected) and `true` otherwise:

```cpp
class settings final : public cse::state
{
  ENLIST(window,
         (display, SDL_DisplayID, {PRIMARY}),
         (position, (std::pair<int, int>), {ORIGIN, ORIGIN}),
         (size, (std::pair<unsigned int, unsigned int>), {1280, 720}),
         (mode, ::mode, {WINDOWED}),
         (vsync, bool, {true}));

  STORE((window, settings::window, {}),
        (volume, double, {0.5}));

  settings() : cse::state({.storage = "settings"}) {};
};
```

Wrap any field whose `type` or `init` contains a top-level comma in parentheses (as `position` and `size` do above) so
the preprocessor reads the tuple correctly. Both macros take the same `(name, type, init)` tuples and both require the
`init`, even when it is just `{}` - a field with no initializer is a compilation error. `ENLIST` supports a few hundred
fields per struct.

State I/O never crashes the game: a file that can't be opened or parsed is renamed to `.bak` and defaults are used, and
a failed write is skipped - both with a logged warning. A field whose JSON value does not match its type is
handled on its own by taking it's default value, and a logged warning tracks this.

Then load and save whenever you want:

```cpp
void window::on_create()
{
  const auto &settings = find_as<csg::settings>(game->active.states, "settings");
  if (!settings->read()) throw cse::exception("Failed to read settings");
  active.width = settings->window.size.first;
  active.mode  = settings->window.mode;
  active.vsync = settings->window.vsync;
}
void window::on_destroy()
{
  const auto &settings = find_as<csg::settings>(game->active.states, "settings");
  settings->window.size  = {active.width, active.height};
  settings->window.mode  = active.mode;
  settings->window.vsync = active.vsync;
  if (!settings->write()) throw cse::exception("Failed to write settings");
}
```

### Localization
Declare the languages you support once with `LANGUAGES`, then declare each translation key once with `TRANSLATE`,
listing every language's value beside it. `LANGUAGES` emits into a `language` namespace and `TRANSLATE` into a `lexeme`
namespace, both nested in whatever namespace you expand them in:

```cpp
namespace custom
{
  LANGUAGES(EN, SP, FR);

  TRANSLATE(welcome_message, (EN, "Welcome!"), (SP, "¡Bienvenido!"), (FR, "Bienvenue!"));
  TRANSLATE(menu_play,       (EN, "Play"),     (SP, "Jugar"),        (FR, "Jouer"));
}
```

That gives you `custom::language::EN` (a `const char *`) and `custom::lexeme::welcome_message` (a translation key).
Text `content` is a `cse::lexeme`, which is any mix of literals and keys that automatically translates based on the
current language:

```cpp
content = "Player";                            // "Player" no matter the current language
content = lexeme::menu_play;                   // Custom translation of "Play"
content = {"[", lexeme::welcome_message, "]"}; // Custom translation of "Welcome!" surrounded by []
```

Assign `game->active.language` to switch; every lexeme holding a key re-resolves on the next frame with no further work
on your part:

```cpp
active.language = language::FR;
```

## Miscellaneous
A grab bag of public utilities the engine exposes.

### Names & Hitboxes
Every `"string"` you pass as an entity/timer/asset name becomes a `cse::name`: a 64-bit FNV-1a hash (computed at
*compile time* in release builds, so lookups and comparisons are integer-cheap; debug builds also keep the original text
for diagnostics). You can build one explicitly from a string or a raw identifier, and call `.string()` /
`.identifier()`. `cse::hitbox` is a `cse::name` plus `left`/`top`/`right`/`bottom` bounds; two hitboxes compare equal
when their names match, regardless of bounds, so contact results can be compared directly against the generated
`hitbox::` constants. Frame data carries hitboxes in texture pixels while collision contacts carry them in world pixels;
either way the coordinates are y-up from the bottom-left, like every pixel coordinate the engine exposes (including
frame pivots).

### Pointer Helpers
Type-safe helpers for the `shared_ptr` entities the engine hands you:
- `lock(weak)` / `try_lock(weak)` - promote a weak pointer (throwing vs. null).
- `is<T...>(ptr)` - exact dynamic-type match (via `typeid`); `is_a<T...>(ptr)` - subclass match (via `dynamic_cast`).
- `as<T>(ptr)` / `as_a<T>(ptr)` - downcast (throwing); `try_as<T>` / `try_as_a<T>` - downcast (null on mismatch).

`as`/`is` pair with exact types; `as_a`/`is_a` pair with polymorphic hierarchies.

### Container Helpers
Beyond `find` / `find_as` shown earlier, the same name-keyed helpers cover the whole matrix - each with a throwing form
and a non-throwing `try_` form:
- `find` / `find_as<T>` - fetch a pointer (optionally downcast).
- `find_is<T...>` / `find_is_a<T...>` - fetch and type-check in one call.
- `contains` - membership test; `iterate` - get the raw iterator.
- `name(container, pointer)` / `try_name(...)` - reverse lookup a pointer (or `weak_ptr`/raw pointer) back to its name.
- `set_or_add` - insert or replace by name.

### Numeric & Bitmask helpers
- `equal(a, b, epsilon = 1e-5)` - robust floating-point comparison (relative epsilon). Used heavily for comparing
  `temporal` values, e.g. `if (equal(active.fov.value, 60.0)) ...`.
- `between(value, min, max)` - inclusive integral range test.
- `cse::axis` (`NONE/X/Y/Z`) and `cse::rectangle` (`left/top/right/bottom`).
- `has(flags, bits)`, plus `any` / `all` / `none` - readable flag testing against SDL-style bitmasks.

### Thread-Safe Printing & Exceptions
- `cse::print<COUT>("hello {}\n", name)` - mutex-guarded, `std::format`-based logging to `COUT` / `CERR` / `CLOG`.
- `cse::log("lost {} frames", n)` - warning log; prints to `CLOG` and appends to `log.txt` in the working directory
  (truncated each run). `cse::sdl_log` appends the current SDL error. The engine uses these for every recoverable
  failure it works around (window management, audio device loss, state I/O, skipped frames).
- `throw cse::exception("bad value {}", x)` - `std::format`-style message; `cse::sdl_exception` appends the current SDL
  error. The engine's `main()` wrapper catches these and reports them, so you can throw freely from any hook.

### System & Shared Constants
- `cse::platform` (`WINDOWS` / `LINUX`), `cse::debug` (`bool`), `cse::success` / `cse::failure` return codes.
- Window/text constants you'll meet in `initial` structs: `PRIMARY` & `ORIGIN` (default display / centered position),
  the `mode` enum (`WINDOWED` / `BORDERLESS` / `FULLSCREEN`), and the text-alignment enums `horizontal`
  (`LEFT/CENTER/RIGHT`) and `vertical` (`TOP/MIDDLE/BOTTOM`).
