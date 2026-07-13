#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"

#include "core.hpp"
#include "function.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "temporal.hpp"
#include "timer.hpp"

namespace cse::help::game
{
  struct tick
  {
    double target{};
    unsigned int count{};
    double average{};
  };
  struct frame
  {
    double target{};
    unsigned int count{};
    double average{};
  };

  struct previous
  {
  public:
    previous() = default;
    previous(const double tick_, const double frame_, const temporal<double> &aspect_, const unsigned int resolution_,
             const temporal<glm::dvec3> &clear_, const temporal<double> &master_, const temporal<double> &sound_,
             const temporal<double> &music_);
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    game::tick tick{};
    game::frame frame{};
    temporal<double> aspect{};
    unsigned int resolution{};
    temporal<glm::dvec3> clear{};
    temporal<double> master{};
    temporal<double> sound{};
    temporal<double> music{};

    std::vector<std::shared_ptr<cse::state>> states{};
    std::shared_ptr<cse::window> window{};
    std::vector<std::shared_ptr<cse::scene>> scenes{};
    std::shared_ptr<cse::scene> scene{};
    std::vector<std::shared_ptr<cse::interface>> interfaces{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};
  };

  struct active
  {
    friend class cse::game;
    friend struct window::active;
    friend struct scene::active;

  private:
    template <typename handle> struct cached
    {
      handle value{};
      double stamp{};
    };

    struct corner
    {
      float x{}, y{};
      float u{}, v{};
    };
    struct graphics_buffer
    {
      SDL_GPUBuffer *vertex{};
      SDL_GPUBuffer *index{};
      SDL_GPUSampler *nearest{};
      SDL_GPUSampler *linear{};
    };
    struct graphics_pipeline
    {
      SDL_GPUGraphicsPipeline *opaque{};
      SDL_GPUGraphicsPipeline *transparent{};
      SDL_GPUGraphicsPipeline *interface{};
    };
    struct graphics_interface
    {
      std::vector<cse::interface *> order{};
    };
    struct graphics_object
    {
      struct batch
      {
        std::size_t first{};
        std::size_t count{};
        SDL_GPUGraphicsPipeline *pipeline{};
        SDL_GPUTexture *texture{};
      };
      struct sample
      {
        std::array<float, 16> model{};
        float red{}, green{}, blue{}, alpha{};
        float left{}, bottom{}, right{}, top{};
        float lit{}, shadowed{}, brightness{}, transparency{};
        float depth{};
        float occluder{-1.0f};
      };
      std::vector<batch> batches{};
      std::vector<sample> samples{};
      std::size_t split{};
      std::pair<glm::dmat4, glm::dmat4> world{};
      std::pair<glm::dmat4, glm::dmat4> overlay{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
    };
    struct graphics_light
    {
      struct header
      {
        std::array<float, 4> meta{};
      };
      struct entry
      {
        std::array<float, 4> position{};
        std::array<float, 4> brightness{};
        std::array<float, 4> direction{};
        std::array<float, 4> cone{};
      };
      header data{};
      std::vector<entry> samples{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
    };
    struct graphics_occluder
    {
      struct entry
      {
        std::array<float, 4> rectangle{};
        std::array<float, 4> frame{};
        std::array<float, 4> surface{};
        std::array<float, 4> shadow{};
      };
      struct layer
      {
        cse::image image{};
        double stamp{};
      };
      std::vector<entry> samples{};
      std::vector<float> indices{};
      std::vector<layer> layers{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
      SDL_GPUTexture *texture{};
      unsigned int width{}, height{};
    };
    struct graphics_cache
    {
      using texture_key = std::pair<const unsigned char *, std::size_t>;
      std::map<texture_key, cached<SDL_GPUTexture *>> texture{};
    };

    struct channel
    {
      const help::mixer *previous{};
      help::mixer *active{};
    };
    struct audio_track
    {
      MIX_Track *handle{};
      MIX_Audio *audio{};
      const unsigned char *source{};
      std::size_t size{};
      double position{};
      double gain{-1.0};
      double speed{-1.0};
      bool started{};
      bool paused{};
      bool finished{};
      bool loop{};
      bool seen{};
    };
    struct audio_cache
    {
      using source_key = std::pair<const unsigned char *, std::size_t>;
      using track_key = std::pair<const void *, std::uint64_t>;
      std::map<source_key, cached<MIX_Audio *>> sources{};
      std::map<track_key, audio_track> tracks{};
    };

  public:
    active() = default;
    active(const double tick_, const double frame_, const temporal<double> &aspect_, const unsigned int resolution_,
           const temporal<glm::dvec3> &clear_, const temporal<double> &master_, const temporal<double> &sound_,
           const temporal<double> &music_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void prepare();
    void create();
    void synchronize(previous &last);
    void render(const temporal<double> previous_aspect);
    void mix(const help::mixer &previous_mixer, const temporal<double> previous_master,
             const temporal<double> previous_sound, const temporal<double> previous_music);
    void destroy();
    void clean();

    void generate_simulation_order();
    void generate_pool();
    bool inside(const glm::dvec2 &position);
    void interact();
    void hover();

    void generate_graphics_order();
    void generate_frustum();
    void generate_lights(const std::vector<cse::light *> &light_order);
    void generate_occluders(const std::vector<cse::object *> &object_order);
    void generate_objects(const std::vector<cse::object *> &object_order);
    void generate_interfaces();
    bool inside_frustum(const glm::dvec3 &center, const double radius) const;
    graphics_pipeline &require_pipelines();
    SDL_GPUTexture *require_texture(const cse::image &image);

    template <typename resource> void reconcile_audio(const help::mixer *previous_mixer, help::mixer *active_mixer,
                                                      const char *tag, const bool predecode, const double bus);
    std::int64_t seconds_to_frames(const double seconds) const;
    double frames_to_seconds(const std::int64_t frames) const;
    static double gain(const double volume);
    MIX_Audio *require_audio(const unsigned char *data, const std::size_t size, const bool predecode);

  public:
    game::tick tick{};
    game::frame frame{};
    temporal<double> aspect{};
    unsigned int resolution{};
    temporal<glm::dvec3> clear{};
    temporal<double> master{};
    temporal<double> sound{};
    temporal<double> music{};

    std::vector<std::shared_ptr<cse::state>> states{};
    std::shared_ptr<cse::window> window{};
    std::vector<std::shared_ptr<cse::scene>> scenes{};
    std::shared_ptr<cse::scene> scene{};
    std::vector<std::shared_ptr<cse::interface>> interfaces{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};

  private:
    double actual_tick{1.0 / tick.target};
    double time{};
    double accumulator{};
    double alpha{};
    std::unordered_set<cse::name> interface_removals{};
    std::vector<std::shared_ptr<cse::interface>> interface_additions{};
    std::vector<cse::interface *> interface_order{};
    std::vector<cse::interface *> interface_pool{};

    static constexpr double cache_lifetime{1.0};

    double actual_frame{1.0 / frame.target};
    SDL_GPUDevice *video{};
    active::graphics_buffer graphics_buffer{};
    active::graphics_pipeline graphics_pipeline{};
    active::graphics_cache graphics_cache{};
    std::array<glm::dvec4, 6> graphics_frustum{};
    active::graphics_light graphics_light{};
    active::graphics_occluder graphics_occluder{};
    active::graphics_object graphics_object{};
    active::graphics_interface graphics_interface{};

    bool audio_ready{};
    int frequency{};
    MIX_Mixer *soundboard{};
    active::audio_cache audio_cache{};
  };

  struct next
  {
  private:
    struct scene
    {
      cse::name name{};
      std::shared_ptr<cse::scene> pointer{};
    };

  public:
    std::optional<std::shared_ptr<cse::window>> window{};
    std::optional<next::scene> scene{};
  };
}

namespace cse
{
  class game
  {
  protected:
    struct initial
    {
      const double tick{100.0};
      const double frame{60.0};
      const temporal<double> aspect{16.0 / 9.0};
      const unsigned int resolution{240};
      const temporal<glm::dvec3> clear{};
      const temporal<double> master{0.5};
      const temporal<double> sound{0.5};
      const temporal<double> music{0.5};
    };

  public:
    virtual ~game() = default;
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <trait::is_game game_type, typename... game_arguments> static std::shared_ptr<game_type>
    create(const std::function<void(const std::shared_ptr<game_type> &)> &config, game_arguments &&...arguments);
    template <trait::is_callable callable, typename... game_arguments>
    static std::shared_ptr<game> create(callable &&config, game_arguments &&...arguments);
    template <trait::is_state state_type, typename... state_arguments>
    state_type &set(const name state_name, state_arguments &&...arguments);
    template <trait::is_window window_type, typename... window_arguments>
    window_type &set(window_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    scene_type &set(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                    scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    auto set(const name scene_name, callable &&config, scene_arguments &&...arguments) ->
      typename trait::callable_smart_inner<callable>::type &;
    template <trait::is_scene scene_type, typename... scene_arguments>
    scene_type &current(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                        scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments> trait::callable_smart_inner<callable>::type &
    current(const name scene_name, callable &&config, scene_arguments &&...arguments);
    scene &current(const name scene_name);
    template <trait::is_interface interface_type, typename... interface_arguments>
    interface_type &set(const name interface_name, interface_arguments &&...arguments);
    template <typename... target_types>
      requires((sizeof...(target_types) == 0) ||
               ((std::is_void_v<target_types> || trait::is_scene<target_types> || trait::is_interface<target_types>) &&
                ...))
    void remove(const name target_name);

    void run();

  protected:
    game(const initial &initial_);
    virtual void pre_prepare();
    virtual void post_prepare();
    virtual void pre_create();
    virtual void post_create();
    virtual void pre_synchronize();
    virtual void post_synchronize();
    virtual void pre_event(const SDL_Event &event);
    virtual void post_event(const SDL_Event &event);
    virtual void pre_simulate(const double tick);
    virtual void post_simulate(const double tick);
    virtual void pre_collide(const double tick);
    virtual void post_collide(const double tick);
    virtual void pre_render(const double alpha);
    virtual void post_render(const double alpha);
    virtual void pre_mix(const double alpha);
    virtual void post_mix(const double alpha);
    virtual void pre_destroy();
    virtual void post_destroy();
    virtual void pre_clean();
    virtual void post_clean();

  private:
    void prepare();
    void create();
    void synchronize();
    void event();
    void simulate();
    void collide();
    void render();
    void mix();
    void destroy();
    void clean();

    void time();
    void step();

    bool running();
    bool behind();
    bool ready();

    void tps();
    void fps();

  public:
    help::game::previous previous{};
    help::game::active active{};
    help::game::next next{};

  private:
    static inline std::weak_ptr<game> instance{};
  };
}

#include "game.inl" // IWYU pragma: keep
