#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "id.hpp"
#include "traits.hpp"

namespace cse
{
  class game final : public std::enable_shared_from_this<game>
  {
  public:
    ~game();
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <help::is_window window_type, typename... window_arguments>
    void set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    void set_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                   scene_arguments &&...arguments);
    template <typename callable, typename... scene_arguments>
    void set_scene(const help::id name, callable &&config, scene_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    void set_current_scene(const help::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                           scene_arguments &&...arguments);
    template <typename callable, typename... scene_arguments>
    void set_current_scene(const help::id name, callable &&config, scene_arguments &&...arguments);
    void set_current_scene(const help::id name);
    bool remove_scene(const help::id name);

    static std::shared_ptr<game> create();
    void run();

  private:
    game() = default;

    void initialize();
    void event();
    void input();
    void simulate();
    void render();
    void cleanup();

    void update_time();
    bool simulation_behind();
    bool should_render();
    void update_fps();

  public:
    std::shared_ptr<class window> window{};
    std::unordered_map<help::id, std::shared_ptr<scene>> scenes{};
    std::weak_ptr<scene> current_scene{};

  private:
    inline static std::weak_ptr<game> instance{};
    static constexpr float global_scale_factor{1.0f / 25.0f};
    static constexpr float target_aspect_ratio{16.0f / 9.0f};
    static constexpr double target_simulation_time{1.0 / 60.0};
    static constexpr double target_render_time{1.0 / 144.0};
    double current_time{};
    double last_simulation_time{};
    double simulation_accumulator{};
    double simulation_alpha{};
    double last_render_time{};
    double last_fps_time{};
    int current_frame_count{};
  };
}

#include "game.inl" // IWYU pragma: keep
