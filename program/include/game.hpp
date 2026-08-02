#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double2.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/ext/vector_double4.hpp"

#include "container.hpp"
#include "core.hpp"
#include "function.hpp"
#include "mixer.hpp"
#include "name.hpp"
#include "resource.hpp"
#include "temporal.hpp"
#include "timer.hpp"

enum scaling : std::uint8_t
{
  VIRTUAL,
  PHYSICAL
};

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
  struct aspect
  {
    double ratio{16.0 / 9.0};
    unsigned int resolution{180};
    ::scaling scaling{VIRTUAL};
  };
  struct vram
  {
    std::size_t current{};
    std::size_t maximum{};
  };
  struct ram
  {
    std::size_t current{};
    std::size_t maximum{};
  };
  struct memory
  {
    struct initial
    {
      std::size_t vram{512};
      std::size_t ram{128};
    };
    game::vram vram{};
    game::ram ram{};
  };

  struct previous
  {
  public:
    previous() = default;
    ~previous() = default;
    previous(const previous &) = delete;
    previous &operator=(const previous &) = delete;
    previous(previous &&) = delete;
    previous &operator=(previous &&) = delete;

  public:
    game::tick tick{};
    game::frame frame{};
    game::aspect aspect{};
    temporal<glm::dvec3> clear{};
    game::memory memory{};
    std::string language{};
    temporal<double> master{};
    temporal<double> sound{};
    temporal<double> music{};

    help::container<cse::state> states{};
    std::shared_ptr<cse::window> window{};
    help::container<cse::scene> scenes{};
    std::shared_ptr<cse::scene> scene{};
    help::container<cse::interface> interfaces{};
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
      std::size_t bytes{};
      double stamp{};
    };
    struct pair_hash
    {
      template <typename first, typename second>
      std::size_t operator()(const std::pair<first, second> &key) const noexcept;
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
      std::vector<double> transparencies{};
      std::vector<char> shown{};
      std::vector<char> lettered{};
      std::vector<std::size_t> emission_order{};
      std::size_t split{};
      std::pair<glm::dmat4, glm::dmat4> world{};
      std::pair<glm::dmat4, glm::dmat4> overlay{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
    };
    struct graphics_text
    {
      struct composed
      {
        double left{}, bottom{}, right{}, top{};
        double uv_left{}, uv_bottom{}, uv_right{}, uv_top{};
      };
      struct item
      {
        const cse::font::glyph *glyph{};
        std::uint32_t character{};
      };
      struct line
      {
        std::size_t first{}, count{};
        double width{};
      };
      struct quad
      {
        std::array<float, 16> model{};
        float left{}, bottom{}, right{}, top{};
        double minimum_x{}, minimum_y{}, maximum_x{}, maximum_y{};
        float occluder{-1.0f};
      };
      struct block
      {
        std::size_t first{}, count{};
        cse::image image{};
        float red{}, green{}, blue{}, alpha{};
        bool visible{};
        bool lit{}, shadowed{}, cast{};
        double brightness{}, transparency{};
        double penetration{}, darkness{}, softness{};
        double plane{};
        int steps{};
      };
      std::vector<composed> scratch{};
      std::vector<std::uint32_t> characters{};
      std::vector<item> items{};
      std::vector<line> lines{};
      std::vector<item> word{};
      std::vector<quad> quads{};
      std::vector<block> blocks{};
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
      std::vector<std::uint8_t> doomed{};
      std::vector<float> remap{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
      SDL_GPUTexture *texture{};
      unsigned int width{}, height{};
    };
    struct graphics_cache
    {
      using texture_key = std::pair<const unsigned char *, std::size_t>;
      std::unordered_map<texture_key, cached<SDL_GPUTexture *>, pair_hash> texture{};
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
      std::unordered_map<source_key, cached<MIX_Audio *>, pair_hash> sources{};
      std::unordered_map<track_key, audio_track, pair_hash> tracks{};
    };

  public:
    active() = default;
    active(const double tick_, const double frame_, const game::aspect &aspect_, const temporal<glm::dvec3> &clear_,
           const game::memory::initial &memory_, const std::string &language_, const temporal<double> &master_,
           const temporal<double> &sound_, const temporal<double> &music_);
    ~active() = default;
    active(const active &) = delete;
    active &operator=(const active &) = delete;
    active(active &&) = delete;
    active &operator=(active &&) = delete;

  private:
    void prepare();
    void create();
    void synchronize(previous &last);
    void render();
    void mix(const help::mixer &previous_mixer, const temporal<double> previous_master,
             const temporal<double> previous_sound, const temporal<double> previous_music);
    void destroy();
    void clean();

    void generate_simulation_order();
    void generate_pool();
    bool inside(const glm::dvec2 &position) const;
    void interact(const SDL_Event &event);
    void hover();

    void generate_graphics_order();
    void generate_frustum();
    void generate_text(const std::vector<cse::object *> &object_order);
    void generate_lights(const std::vector<cse::light *> &light_order);
    void generate_occluders(const std::vector<cse::object *> &object_order);
    void generate_objects(const std::vector<cse::object *> &object_order);
    void generate_interfaces();
    bool inside_frustum(const glm::dvec3 &center, const double radius) const;
    static bool usable(const cse::image &image);
    template <typename type> void compose_text(type &text, const type &last, const cse::name &element,
                                               const double box_left, const double box_right, const double box_top,
                                               const double box_bottom, std::vector<graphics_text::composed> &output);
    graphics_pipeline &require_pipelines();
    SDL_GPUTexture *require_texture(const cse::image &image);

    template <typename resource> void reconcile_audio(const help::mixer *previous_mixer, help::mixer *active_mixer,
                                                      const char *tag, const bool predecode, const double bus);
    std::int64_t seconds_to_frames(const double seconds) const;
    double frames_to_seconds(const std::int64_t frames) const;
    MIX_Audio *require_audio(const unsigned char *data, const std::size_t size, const bool predecode);

  public:
    game::tick tick{};
    game::frame frame{};
    game::aspect aspect{};
    temporal<glm::dvec3> clear{};
    game::memory memory{};
    std::string language{};
    temporal<double> master{};
    temporal<double> sound{};
    temporal<double> music{};

    help::container<cse::state> states{};
    std::shared_ptr<cse::window> window{};
    help::container<cse::scene> scenes{};
    std::shared_ptr<cse::scene> scene{};
    help::container<cse::interface> interfaces{};
    help::timer timer{};
    help::mixer mixer{};
    help::phase phase{};

  private:
    double actual_tick{1.0 / tick.target};
    double time{};
    double accumulator{};
    double alpha{};
    std::unordered_set<cse::name> interface_removals{};
    help::container<cse::interface> interface_additions{};
    std::vector<cse::interface *> interface_order{};
    std::vector<cse::interface *> interface_pool{};

    double actual_frame{1.0 / frame.target};
    game::vram actual_vram{};
    game::ram actual_ram{};
    SDL_GPUDevice *video{};
    active::graphics_buffer graphics_buffer{};
    active::graphics_pipeline graphics_pipeline{};
    active::graphics_cache graphics_cache{};
    std::array<glm::dvec4, 6> graphics_frustum{};
    active::graphics_light graphics_light{};
    active::graphics_occluder graphics_occluder{};
    active::graphics_object graphics_object{};
    active::graphics_text graphics_text{};
    active::graphics_interface graphics_interface{};

    bool audio_ready{};
    int frequency{};
    MIX_Mixer *soundboard{};
    active::audio_cache audio_cache{};
    std::vector<active::channel> audio_channels{};
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
      const help::meta::initial meta{};
      const double tick{100.0};
      const double frame{144.0};
      const help::game::aspect aspect{};
      const temporal<glm::dvec3> clear{};
      const help::game::memory::initial memory{};
      const std::string language{};
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
    auto set(const name scene_name, callable &&config, scene_arguments &&...arguments)
      -> trait::callable_smart_inner<callable>::type &;
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

    SDL_AppResult initialize();
    SDL_AppResult receive(const SDL_Event &event);
    SDL_AppResult iterate();
    void quit();

  protected:
    explicit game(const initial &initial_);
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

    bool running() const;
    bool behind();
    bool ready();

    void tps();
    void fps();

  public:
    help::game::previous previous{};
    help::game::active active{};
    help::game::next next{};

  private:
    static inline std::shared_ptr<game> instance{};
    std::vector<SDL_Event> queue{};
  };
}

#include "game.inl" // IWYU pragma: keep
