#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "declaration.hpp"

namespace cse::core
{
  class game
  {
  public:
    game();
    ~game();
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    void run();

    template <typename window_type, typename... window_arguments> void set_window(window_arguments &&...arguments);
    std::weak_ptr<scene> get_scene(const std::string &name) const;
    template <typename scene_type, typename... scene_arguments>
    std::weak_ptr<scene> add_scene(const std::string &name, scene_arguments &&...arguments);
    void set_current_scene(const std::string &name);

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
    std::unique_ptr<class window> window = {};
    std::unordered_map<std::string, std::shared_ptr<scene>> scenes = {};
    std::weak_ptr<scene> current_scene = {};

    const double target_simulation_time = 1.0 / 60.0;
    double last_simulation_time = {};
    double simulation_accumulator = {};
    double simulation_alpha = {};
    const double target_render_time = 1.0 / 144.0;
    double last_render_time = {};
    double last_fps_time = {};
    int frame_count = {};
    const float scale_factor = 0.04f;
  };
}

#include "game.inl"
