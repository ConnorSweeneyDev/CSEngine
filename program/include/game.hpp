#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "SDL3_ttf/SDL_ttf.h"
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
    temporal<double> master{0.5};
    temporal<double> sound{0.5};
    temporal<double> music{0.5};

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
    friend struct scene::active;

  private:
    struct corner
    {
      float x{}, y{};
      float u{}, v{};
    };
    struct graphics_buffer
    {
      SDL_GPUBuffer *vertex{};
      SDL_GPUBuffer *index{};
      SDL_GPUSampler *sample{};
    };
    struct pipeline
    {
      SDL_GPUGraphicsPipeline *opaque{};
      SDL_GPUGraphicsPipeline *transparent{};
      SDL_GPUGraphicsPipeline *interface{};
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
        float transparency{};
      };
      std::vector<batch> batches{};
      std::vector<sample> samples{};
      std::size_t capacity{};
      SDL_GPUBuffer *buffer{};
      SDL_GPUTransferBuffer *transfer_buffer{};
    };
    struct graphics_interface
    {
      struct label
      {
        std::string text{};
        const unsigned char *font{};
        unsigned int size{};
        TTF_FontStyleFlags style{};
        glm::dvec4 color{};
        TTF_HorizontalAlignment align{};
        bool wrap{};
        unsigned int width{};
        int skip{};
        SDL_GPUTexture *texture{};
        unsigned int texture_width{};
        unsigned int texture_height{};
        std::uint64_t stamp{};
      };
      std::vector<cse::interface *> order{};
      std::map<const cse::interface *, label> labels{};
      std::uint64_t stamp{};
    };
    struct graphics_cache
    {
      using pipeline_key = std::tuple<const unsigned char *, std::size_t, const unsigned char *, std::size_t>;
      using texture_key = std::pair<const unsigned char *, std::size_t>;
      using font_key = std::tuple<const unsigned char *, std::size_t, unsigned int>;
      struct typeface
      {
        TTF_Font *font{};
        int skip{};
      };
      template <typename handle> struct cached
      {
        handle value{};
        std::uint64_t stamp{};
      };
      std::map<pipeline_key, cached<active::pipeline>> pipeline{};
      std::map<texture_key, cached<SDL_GPUTexture *>> texture{};
      std::map<font_key, cached<typeface>> font{};
      std::uint64_t stamp{};
    };

    struct channel
    {
      const help::mixer *previous{};
      help::mixer *active{};
    };
    struct audio_track
    {
      using handle_key = std::pair<const void *, std::uint64_t>;
      using audio_key = std::pair<const unsigned char *, std::size_t>;
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
    void generate_object_samples_and_batches(const std::vector<cse::object *> &object_order);
    void generate_interface_samples_and_batches();
    void upload_samples();
    void draw_batches(const std::pair<glm::dmat4, glm::dmat4> &matrices);
    pipeline &require_pipelines(const cse::vertex &vertex, const cse::fragment &fragment);
    SDL_GPUTexture *require_texture(const cse::image &image);
    graphics_cache::typeface &require_font(const cse::font &font, const unsigned int size);
    graphics_interface::label &require_label(const cse::interface *element);

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
    temporal<double> master{0.5};
    temporal<double> sound{0.5};
    temporal<double> music{0.5};

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

    double actual_frame{1.0 / frame.target};
    active::graphics_buffer graphics_buffer{};
    active::graphics_cache graphics_cache{};
    active::graphics_object graphics_object{};
    active::graphics_interface graphics_interface{};

    int frequency{};
    MIX_Mixer *audio_handle{};
    std::map<audio_track::audio_key, MIX_Audio *> audio_cache{};
    std::map<audio_track::handle_key, audio_track> audio_tracks{};
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
      const double tick{};
      const double frame{};
      const temporal<double> aspect{};
      const unsigned int resolution{};
      const temporal<glm::dvec3> clear{};
      const temporal<double> master{};
      const temporal<double> sound{};
      const temporal<double> music{};
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
    template <trait::is_window window_type, typename... window_arguments> game &set(window_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &set(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
              scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &set(const name scene_name, callable &&config, scene_arguments &&...arguments);
    template <trait::is_scene scene_type, typename... scene_arguments>
    game &current(const name scene_name, const std::function<void(const std::shared_ptr<scene_type> &)> &config,
                  scene_arguments &&...arguments);
    template <trait::is_callable callable, typename... scene_arguments>
    game &current(const name scene_name, callable &&config, scene_arguments &&...arguments);
    game &current(const name scene_name);
    template <trait::is_interface interface_type, typename... interface_arguments>
    game &set(const name interface_name, interface_arguments &&...arguments);
    template <typename target_type = void>
      requires(std::is_void_v<target_type> || trait::is_scene<target_type> || trait::is_interface<target_type>)
    game &remove(const name target_name);

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
