<div align="center">

# CSEngine

**A game engine for creating 2D/3D hybrid pixel art games with ease.**

<video src="https://github.com/user-attachments/assets/4aec7a39-614f-448f-8a55-f4d76f9b3eb5"></video>
<em>A video of a dimly lit scene with text, audio, a custom cursor, an animated sprite and soft shadows.</em>

</div>

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
Define `cse::main` - the engine provides the real `main()` and wraps your code in error handling. Create the game from a
setup function and run it:

```cpp
#include "cse/main.hpp"
#include "cse/game.hpp"
#include "game.hpp"

int cse::main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw exception("Expected 1 argument, got {}", argc);
  game::create(custom::game::setup)->run();
  return success;
}
```

### Composing the Game
Subclass `cse::game` and set tuning defaults in the constructor (meta info, tick rate, frame rate, aspect ratio, virtual
pixel resolution, clear colour, audio buses):

```cpp
namespace custom
{
  class game final : public cse::game
  {
  public:
    game();
    static void setup(const std::shared_ptr<game> &game);

  protected:
    void pre_create() override final;
    void pre_event(const SDL_Event &event) override final;
    void pre_simulate(const double tick) override final;
    void post_destroy() override final;
  };

  game::game()
    : cse::game({.meta = {.organization = "ConnorSweeneyDev", .application = "CSGame", .version = "1.0.0"},
                 .tick = 300.0,
                 .frame = 144.0,
                 .aspect = {.value = 16.0 / 9.0, .interpolate = true},
                 .resolution = 100,
                 .clear = {.value = {0.0, 0.0, 0.0}, .interpolate = true},
                 .master = {.value = 0.5, .interpolate = true},
                 .sound = {.value = 1.0, .interpolate = true},
                 .music = {.value = 1.0, .interpolate = true}})
  {
  }
}
```

The `setup` function builds the whole entity tree with a fluent, chained API. `set<...>` registers an entity; `current`
registers a scene *and* makes it the active one, but calling it without a config function just switches to an
already-registered scene:

```cpp
void game::setup(const std::shared_ptr<game> &game)
{
  game->set<settings>("settings")
       .set<window>()
       .current("main", scene::main)
       .set<cursor>("cursor");
}
```

A scene's contents are populated by a config function passed to `set`/`current`. Note `set<camera>` takes no name (a
scene has exactly one), while objects, lights and interfaces are named:

```cpp
void scene::main(const std::shared_ptr<scene> &scene)
{
  scene->set<button>("button1", glm::dvec2{-22.0, -40.0})
        .set<camera>(glm::dvec3{0.0, 0.0, 80.0})
        .set<player>("player", glm::dvec3{0.0, 0.0, 0.0})
        .set<environment>("floor", glm::dvec3{0.0, -61.0, 0.0}, image::floor, animation::floor.main)
        .set<sun>("sun");
}
```

You can swap scenes at runtime with `game->current("name")` (re-use a registered scene) or
`game->current("name", configFn)` (build it on the fly), and add/remove entities live with `scene->set<...>(...)` /
`scene->remove("name")`.

### Defining Entities
Every entity type follows the same shape: subclass the engine base, pass an `initial` struct (designated initializers)
to the base constructor, and override the hooks you care about. An object:

```cpp
class player final : public cse::object
{
public:
  player(const glm::dvec3 &translation_);

protected:
  void on_event(const SDL_Event &event) override final;
  void on_simulate(const double tick) override final;
};

player::player(const glm::dvec3 &translation_)
  : cse::object({.translation = {.value = translation_, .interpolate = true},
                 .rotation = {.value = 0.0, .interpolate = true},
                 .scale = {.value = {1.0, 1.0}, .interpolate = true},
                 .collidable = true,
                 .texture = {.image = image::redhood,
                             .animation = animation::redhood.idle,
                             .playback = {.frame = 0, .speed = {1.0}, .loop = true},
                             .color = {.tint = {.value = {0.5, 0.5, 0.5, 1.0}, .interpolate = true},
                                       .alpha = {.value = 1.0, .interpolate = true}}},
                 .illumination = {.show = true,
                                  .brightness = {.value = 1.0, .interpolate = true},
                                  .penetration = {.value = 1.0, .interpolate = true}},
                 .shadow = {.show = true,
                            .cast = true,
                            .darkness = {.value = 1.0, .interpolate = true},
                            .softness = {.value = 1.0, .interpolate = true}},
                 .priority = {.simulation = 0, .rendering = 1}})
{
}
```

`camera`, `light`, `interface` and `window` are defined the same way against their own `initial` structs (see CSGame's
`camera.cpp`, `light.cpp`, `interface.cpp`, `window.cpp`).

### Lifecycle Hooks
Hooks are virtual no-ops you override. **Leaf entities** (`window`, `interface`, `camera`, `object`, `light`) expose
`on_*` hooks; **`game` and `scene`** expose `pre_*`/`post_*` hooks that bracket the work of their children.

The best way to describe the lifecycle of an entity is to show you the game's main loop:

```cpp
  void game::run()
  {
    prepare();
    create();
    while (running())
    {
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
    }
    destroy();
    clean();
  }
```

For the most part, every entity follows this lifecycle - the only difference being that any of the core classes other
than game don't necessarily call prepare and create/destroy and clean one after another - prepare is called at the
instantiation of the entity in memory, and create is called when the entity becomes active in a scene. A similar
behaviour describes destroy and clean.

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

### Accessing State at Runtime
- `active.*` is the current snapshot; `previous.*` is last tick's - compare them to detect transitions.
- Reach related entities through pointers: every entity has `game`, and scene-owned entities also have `scene`. So
  `scene->game->active.window->active.keyboard`, or from a game-level interface, `game->active.window->active.mouse`.
- Look entities up by name with the helpers in `cse/container.hpp`:
  ```cpp
  find(active.interfaces, "tick")->active.text.content = "TPS:" + std::to_string(active.tick.count);
  auto player = try_find(active.objects, "player");                     // nullptr if absent
  auto settings = find_as<custom::settings>(active.states, "settings"); // find + downcast
  if (is<player>(contact.target.pointer)) { ... }                       // exact-type check
  ```
  Each comes in a throwing form (`find`, `find_as`, `contains`) and a non-throwing `try_` form.

### Starting and Calling Timers
Schedule one-shot or repeating callbacks on any entity's `active.timer`. `set` returns the timer's modifiable `state`.
`call` returns whether it fired (discards any callback return). `capture` is for non-void callbacks and returns
`optional<return_type>`:

```cpp
// Set a timer for 0.5 seconds that clears the text content when it fires
auto &hide = active.timer.set("hide_text", [this]() { active.text.content.clear(); });
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
sfx.position = 0;
sfx.playing = true;
active.mixer.remove<cse::music>("main");
```

Removing is optional; the mixer will automatically clear tracks when the entity is destroyed.\
The game's `master`, `sound` and `music` temporals act as global buses for volume, all affecting each track's own
`volume` temporal.

### Persistent State
A `state` is a JSON-backed settings blob saved under the OS user-data directory. Declare fields with the `ENLIST` macro
(it builds a plain struct *and* its JSON serializers in one place - pass each field as a `(name, type, init)` tuple, add
as many as you like), expose them with `FIELD`, and call `read()`/`write()`:

```cpp
class settings final : public cse::state
{
private:
  ENLIST(window,
         (display, SDL_DisplayID, {PRIMARY}),
         (position, (std::pair<int, int>), {ORIGIN, ORIGIN}),
         (size, (std::pair<unsigned int, unsigned int>), {1280, 720}),
         (mode, ::mode, {WINDOWED}),
         (vsync, bool, {true}));

public:
  settings() : cse::state({.storage = "settings"}) {}
  FIELD(window, settings::window, {});
};
```

Wrap any field whose `type` or `init` contains a top-level comma in parentheses (as `position` and `size` do above) so
the preprocessor reads the tuple correctly. `ENLIST` supports a few hundred fields per struct.

State I/O never crashes the game: a file that can't be opened or parsed is renamed to `.bak` and defaults are used, and
a failed write is skipped - both with a logged warning.

Then load and save whenever you want:

```cpp
void window::on_create()
{
  const auto &settings = find_as<csg::settings>(game->active.states, "settings");
  active.width = settings->window->size.first;
  active.mode  = settings->window->mode;
  active.vsync = settings->window->vsync;
}
void window::on_destroy()
{
  const auto &settings = find_as<csg::settings>(game->active.states, "settings");
  settings->window->size = {active.width, active.height};
  settings->window->mode = active.mode;
  settings->write();
}
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
