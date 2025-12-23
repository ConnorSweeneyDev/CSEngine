#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "glm/ext/vector_uint2.hpp"

#include "declaration.hpp"
#include "id.hpp"

namespace cse::core
{
  class game
  {
  public:
    game() = default;
    ~game();
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    std::shared_ptr<window> get_window() const noexcept;
    template <typename window_type> std::shared_ptr<window_type> get_window() const noexcept;
    std::shared_ptr<window> get_window_strict() const;
    template <typename window_type> std::shared_ptr<window_type> get_window_strict() const;
    template <typename window_type, typename... window_arguments>
    void set_window(const std::string &title, const glm::uvec2 &dimensions, window_arguments &&...arguments);

    std::shared_ptr<scene> get_scene(const helper::id name) const noexcept;
    template <typename scene_type> std::shared_ptr<scene_type> get_scene(const helper::id name) const noexcept;
    std::shared_ptr<scene> get_scene_strict(const helper::id name) const;
    template <typename scene_type> std::shared_ptr<scene_type> get_scene_strict(const helper::id name) const;
    template <typename scene_type, typename... scene_arguments>
    void add_scene(const helper::id name, const std::function<void(const std::shared_ptr<scene_type>)> &config,
                   scene_arguments &&...arguments);

    std::pair<helper::id, std::shared_ptr<scene>> get_current_scene() const noexcept;
    template <typename scene_type>
    std::pair<helper::id, std::shared_ptr<scene_type>> get_current_scene() const noexcept;
    std::pair<helper::id, std::shared_ptr<scene>> get_current_scene_strict() const;
    template <typename scene_type> std::pair<helper::id, std::shared_ptr<scene_type>> get_current_scene_strict() const;
    void set_current_scene(const helper::id name);

    void run();

  private:
    void initialize();
    void event();
    void input();
    void simulate();
    void render();
    void cleanup();

    void update_simulation_time();
    bool simulation_behind();
    void update_simulation_alpha();
    bool should_render();
    void update_fps();

  private:
    std::shared_ptr<class window> window{};
    std::unordered_map<helper::id, std::shared_ptr<scene>> scenes{};
    std::weak_ptr<scene> current_scene{};

    static constexpr float global_scale_factor{1.0f / 25.0f};
    static constexpr float target_aspect_ratio{16.0f / 9.0f};
    static constexpr double target_simulation_time{1.0 / 60.0};
    static constexpr double target_render_time{1.0 / 144.0};
    double last_simulation_time{};
    double simulation_accumulator{};
    double simulation_alpha{};
    double last_render_time{};
    double last_fps_time{};
    int current_period_frame_count{};
  };
}

#include "game.inl"
