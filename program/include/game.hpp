#pragma once

#include <memory>
#include <unordered_map>

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

    void run();
    template <typename window_type, typename... window_arguments> void set_window(window_arguments &&...arguments);
    std::weak_ptr<scene> get_scene(const helper::id name) const;
    template <typename scene_type, typename... scene_arguments>
    std::weak_ptr<scene> add_scene(const helper::id name, scene_arguments &&...arguments);
    void set_current_scene(const helper::id name);

  private:
    void initialize();
    void cleanup();
    void event();
    void input();
    void simulate();
    void render();

    void update_simulation_time();
    bool simulation_behind();
    void update_simulation_alpha();
    bool should_render();
    void update_fps();

  private:
    std::unique_ptr<class window> window{};
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
