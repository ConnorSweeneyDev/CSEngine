<div align="center">

# CSEngine

**A game engine for creating 2D/3D hybrid pixel art games with ease.**

<img width="800" alt="Example scene made with CSEngine"
     src="https://github.com/user-attachments/assets/2fe9201a-149b-4b5f-9306-2792ce31ea29" />

<em>A screenshot of a dimly lit scene with text, a custom cursor, an animated sprite and soft shadows.</em>

</div>

## Usage
CSEngine is a static library (`cse`) that you link into your own executable. You describe your game declaratively as a
tree of named entities, override lifecycle hooks to add behaviour, and the engine drives a fixed-timestep simulation
with an interpolated render loop on top of SDL3 (GPU, TTF and Mixer).

The canonical, complete example of everything below is the [CSGame](https://github.com/ConnorSweeneyDev/CSGame) project
— it is the reference consumer and a good template to copy.

### Mental Model
- **A `game`** owns a persistent `state`, a `window`, a current `scene`, and a variable amount of `scene`s and
  `interface`s.
- **A `scene`** owns one `camera`, and a variable amount of `interface`s, `object`s and `light`s.
- **An `interface`** is a 2D overlay (HUD, menu, etc.), whereas a `camera`/`object`/`light` is a 3D entity in the scene.
- **Entities are added by name** (`"player"`, `"button1"`, …) and looked up by name later.
- **You never write a `main` loop.** You subclass the engine types, override `on_*`/`pre_*`/`post_*` hooks, and the
  engine calls them in the right order each tick/frame.
- **Simulation is fixed-timestep** (e.g. 300Hz); **rendering is decoupled and interpolated** (e.g. 144fps). Values that
  change over time are declared as `temporal<T>` so the renderer can smoothly interpolate between the previous and
  current state, and to make it easier to drive them without extra state.
- **Everything has a current (`active`) and previous (`previous`) snapshot**, so behaviour can compare "this tick vs
  last tick" (e.g. detecting a hover that just started).

### Project Layout
An example project layout could look like this:

```
your-game/
  csb/                   # CSBuild bootstrap (csb.cpp, csb.hpp, script/build.bat|sh)
  program/
    include/             # your headers
    source/              # your sources
    vertex/   *.vert     # HLSL vertex shaders
    fragment/ *.frag     # HLSL fragment shaders
    texture/  *.aseprite # sprite sheets (with animation/hitbox metadata)
    font/     *.ttf      # fonts
    sound/    *.wav      # short sound effects
    music/    *.opus     # streamed music
```

However, you are not tied to anything - you can use any build system and layout you like, as long as you satisfy the
requirements below.

### Build System Jobs
After pulling in CSEngine, it builds with [CSBuild](https://github.com/ConnorSweeneyDev/CSBuild) (run
`csb\script\build.bat` on Windows or `./csb/script/build.sh` on Linux from the project directory). Your build system
should do the following:

1. **Includes and Library Paths.** All library headers will be copied into `build/include`, and all libraries will be
   copied into `build/[release|debug]`. It is your job to include the headers and link the libraries.
2. **Required Compilation Flags.** On MSVC, compile with at least `/std:c++20`, and `/bigobj`, `/Zc:preprocessor`.
3. **Compiling Shaders to SPIR-V.** HLSL needs to be compiled (e.g. with dxc: `dxc -spirv -T [vs_6_0|ps_6_0] -E main`)
   into `.spv`. Each shader's entry point is `main`.
4. **Packing & Embedding Assets.** Compiled shaders, fonts, `.aseprite` textures, `.wav` and `.opus` audio are packed
   into `.csp` containers (via [CSPack](https://github.com/ConnorSweeneyDev/CSPack)) and a `resource.hpp`/`resource.cpp`
   pair is generated that exposes every asset as a typed C++ symbol in your namespace - the header should be in this
   form:

   ```cpp
   namespace csg
   {
     namespace vertex { extern const cse::vertex main; ... }
     namespace fragment { extern const cse::fragment main; ... }
     namespace font { extern const cse::font main; ... }
     namespace image { extern const cse::image main; ... }
     namespace animation
     {
       namespace detail
       {
         struct main_animation
         {
           const cse::animation idle;
           ...
         };
         ...
       }
       extern const detail::main_animation main;
     }
     namespace hitbox
     {
       namespace detail
       {
         struct main_hitbox
         {
           const cse::hitbox body;
           ...
         };
         ...
       }
       extern const detail::main_hitbox main;
     }
     namespace sound { extern const cse::sound main; ... }
     namespace music { extern const cse::music main; ... }
   }
   ```

   The source should be in this form:

   ```cpp
   namespace cse::resource
   {
     const loader loaded_pack{"pack.csp", [signature], [frames_offset], [frames_size], [hitboxes_offset],
                              [hitboxes_size], [strings_offset]};
     ...
   }

   namespace csg
   {
     namespace vertex { const cse::vertex main{cse::resource::region("pack.csp", [offset], [size])}; ... }
     namespace fragment { const cse::fragment main{cse::resource::region("pack.csp", [offset], [size])}; ... }
     namespace font { const cse::font main{cse::resource::region("pack.csp", [offset], [size])}; ... }
     namespace image
     {
       const cse::image main{cse::resource::region("pack.csp", [offset], [size]),
                             [width], [height], [frame_width], [frame_height], [channels]};
       ...
     }
     namespace animation
     {
       const detail::main_animation main{{cse::resource::frames("pack.csp", [frame_offset], [frame_count),
                                          [from], [to]}};
       ...
     }
     namespace hitbox { const detail::main_hitbox main{"main.body", ...}; ... }
     namespace sound { const cse::sound main{cse::resource::region("pack.csp", [offset], [size])}; ... }
     namespace music { const cse::music main{cse::resource::region("pack.csp", [offset], [size])}; ... }
   }
   ```

   The `loader`'s trailing `[strings_offset]` argument exists only in debug builds (where hitbox identifiers are stored
   as readable strings); release builds omit it and hash the identifiers instead.

### Entry Point
Define `cse::main` — the engine provides the real `main()` and wraps your code in error handling. Create the game from a
setup function and run it:

```cpp
#include "cse/main.hpp"
#include "cse/game.hpp"
#include "game.hpp"

int cse::main(int argc, char *argv[])
{
  if (argc > 1 || !argv[0]) throw exception("Expected 1 argument, got {}", argc);
  game::create(csg::game::setup)->run();
  return success;
}
```

### Composing the game
Subclass `cse::game` and set tuning defaults in the constructor (tick rate, frame rate, aspect ratio, virtual pixel
resolution, clear colour, audio buses):

```cpp
namespace csg
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
    : cse::game({.tick = 300.0,
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

### Defining entities
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
                 .shader = {.vertex = vertex::main, .fragment = fragment::main},
                 .texture = {.image = image::redhood,
                             .animation = animation::redhood.idle,
                             .playback = {.frame = 0, .speed = {1.0}, .loop = true},
                             .color = {.value = {0.5, 0.5, 0.5, 1.0}, .interpolate = true},
                             .transparency = {.value = 1.0, .interpolate = true}},
                 .illumination = {.show = true, .brightness = {.value = 1.0, .interpolate = true}},
                 .shadow = {.cast = true, .show = true,
                            .darkness = {.value = 1.0, .interpolate = true},
                            .softness = {.value = 1.0, .interpolate = true}},
                 .priority = {.simulation = 0, .rendering = 1}})
{
}
```

`camera`, `light`, `interface` and `window` are defined the same way against their own `initial` structs (see CSGame's
`camera.cpp`, `light.cpp`, `interface.cpp`, `window.cpp`).

### Lifecycle hooks
Hooks are virtual no-ops you override. **Leaf entities** (`window`, `scene`'s camera/objects/lights/interfaces) expose
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

### The temporal value model
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

### Accessing state at runtime
- `active.*` is the current snapshot; `previous.*` is last tick's — compare them to detect transitions.
- Reach related entities through pointers: every entity has `game`, and scene-owned entities also have `scene`. So
  `scene->game->active.window->active.keyboard`, or from a game-level interface, `game->active.window->active.mouse`.
- Look entities up by name with the helpers in `cse/container.hpp`:
  ```cpp
  find(active.interfaces, "tick")->active.text.content = "TPS:" + std::to_string(active.tick.count);
  auto player = try_find(active.objects, "player");                  // nullptr if absent
  auto settings = find_as<csg::settings>(active.states, "settings"); // find + downcast
  if (is<player>(contact.target.pointer)) { ... }                    // exact-type check
  ```
  Each comes in a throwing form (`find`, `find_as`, `contains`) and a non-throwing `try_` form.

### Timers
Schedule one-shot or deferred callbacks on any entity's `active.timer`:

```cpp
active.timer.set("hide_text", 0.5, [this]() { active.text.content.clear(); }); // fire after 0.5s
active.timer.poll<void()>("hide_text");                                        // run it when due
```

### Audio
Each game, scene and entity has a `mixer`. Load by name, then toggle/seek/pitch:

```cpp
auto &song = active.mixer.load("main", music::main);
song.playing = true;
song.loop = true;
song.speed.value = 0.80;

active.mixer.load({{"sample1", sound::sample1}, {"sample2", sound::sample2}});
auto &sfx = active.mixer.get<cse::sound>("sample1");
sfx.position = 0; sfx.playing = true;
active.mixer.unload<cse::music>("main");
```

Unloading is optional; the mixer will automatically unload when the entity is destroyed.
The game's `master`, `sound` and `music` temporals act as global buses for volume, as well as each track's own `volume`
temporal.

### Persistent settings (state)
A `state` is a JSON-backed settings blob saved under the OS user-data directory. Declare fields with the `ENLIST` macro
(it builds a plain struct *and* its JSON serializers in one place — pass each field as a `(name, type, init)` tuple, add
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
  settings() : cse::state("CSGame/settings") {}
  FIELD(settings::window, window, {});
};
```

Wrap any field whose `type` or `init` contains a top-level comma in parentheses (as `position` and `size` do above) so
the preprocessor reads the tuple correctly. `ENLIST` supports a few hundred fields per struct.

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

### Strongly-typed enumerations (`cse/enumeration.hpp`)
`cse::enumeration<derived>` is a CRTP base for building open, strongly-typed enum-like sets of values — handy when a
plain `enum` is too rigid (e.g. you want to inherit from an existing enum). Each default-constructed `value` grabs the
next integer; passing an explicit count reseeds the counter:

```cpp
struct colour : cse::enumeration<colour>
{
  static inline const value red{};   // 0
  static inline const value green{}; // 1
  static inline const value blue{};  // 2
};
struct more_colour : colour
{
  static inline const value yellow{}; // 3
  static inline const value cyan{};   // 4
};

auto chosen = more_colour::yellow;
if (chosen == colour::green) { ... }
int index = chosen;
```

Values are equality- and `<=>`-comparable, and the `enumeration_value` concept lets you constrain templates to them.

### Observable values (`cse/property.hpp`)
`cse::property<T>` is a transparent wrapper that behaves exactly like the `T` it holds — every operator
(`= += * == <=> ++` …), `operator->`/`*`, plus `std::formatter`, `std::hash` and `operator>>` are forwarded — but it
also carries a `std::function<void()> change` callback you can set to react to mutations. Use it when a field needs a
side effect on assignment without writing a getter/setter pair.

### Names & hitboxes (`cse/name.hpp`)
Every `"string"` you pass as an entity/timer/asset name becomes a `cse::name`: a 64-bit FNV-1a hash (computed at
*compile time* in release builds, so lookups and comparisons are integer-cheap; debug builds also keep the original text
for diagnostics). `cse::hitbox` is just an alias of `cse::name`. You can build one explicitly from a string or a raw
identifier, and call `.string()` / `.identifier()`.

### Pointer & cast helpers (`cse/pointer.hpp`)
Type-safe helpers for the `shared_ptr` entities the engine hands you:
- `lock(weak)` / `try_lock(weak)` — promote a weak pointer (throwing vs. null).
- `is<T...>(ptr)` — exact dynamic-type match (via `typeid`); `is_a<T...>(ptr)` — subclass match (via `dynamic_cast`).
- `as<T>(ptr)` / `as_a<T>(ptr)` — downcast (throwing); `try_as<T>` / `try_as_a<T>` — downcast (null on mismatch).

`as`/`is` pair with exact types; `as_a`/`is_a` pair with polymorphic hierarchies.

### Container lookups (`cse/container.hpp`)
Beyond `find` / `find_as` shown earlier, the same name-keyed helpers cover the whole matrix — each with a throwing form
and a non-throwing `try_` form:
- `find` / `find_as<T>` — fetch a pointer (optionally downcast).
- `find_is<T...>` / `find_is_a<T...>` — fetch and type-check in one call.
- `contains` — membership test; `iterate` — get the raw iterator.
- `name(container, pointer)` / `try_name(...)` — reverse lookup a pointer (or `weak_ptr`/raw pointer) back to its name.
- `set_or_add` — insert or replace by name.

### Numeric & bitmask helpers (`cse/numeric.hpp`, `cse/mask.hpp`)
- `equal(a, b, epsilon = 1e-5)` — robust floating-point comparison (relative epsilon). Used heavily for comparing
  `temporal` values, e.g. `if (equal(active.fov.value, 60.0)) ...`.
- `between(value, min, max)` — inclusive integral range test.
- `cse::axis` (`NONE/X/Y/Z`) and `cse::rectangle` (`left/top/right/bottom`).
- `has(flags, bits)`, plus `any` / `all` / `none` — readable flag testing against SDL-style bitmasks.

### Thread-safe printing & exceptions (`cse/print.hpp`, `cse/exception.hpp`)
- `cse::print<COUT>("hello {}\n", name)` — mutex-guarded, `std::format`-based logging to `COUT` / `CERR` / `CLOG`.
- `throw cse::exception("bad value {}", x)` — `std::format`-style message; `cse::sdl_exception` appends the current SDL
  error. The engine's `main()` wrapper catches these and reports them, so you can throw freely from any hook.

### System & shared constants (`cse/system.hpp`)
- `cse::platform` (`WINDOWS` / `LINUX`), `cse::debug` (`bool`), `cse::success` / `cse::failure` return codes.
- Window/text constants you'll meet in `initial` structs: `PRIMARY` & `ORIGIN` (default display / centered position),
  the `mode` enum (`WINDOWED` / `BORDERLESS` / `FULLSCREEN`), and the text-alignment enums `horizontal`
  (`LEFT/CENTER/RIGHT`) and `vertical` (`TOP/MIDDLE/BOTTOM`).
