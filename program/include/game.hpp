#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "scene.hpp"
#include "window.hpp"

namespace cse
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
    template <typename scene_type, typename... scene_arguments>
    void add_scene(const std::string &name, scene_arguments &&...arguments);
    void set_current_scene(const std::string &name);
    std::weak_ptr<base::scene> get_scene(const std::string &name) const;

  private:
    void initialize();
    void cleanup();
    void input();
    void simulate();
    void render();

    void update_simulation_time();
    bool simulation_behind();
    void update_simulation_alpha();
    bool should_render();
    void update_fps();

  public:
    inline static const float scale_factor = 0.04f;

  private:
    std::unique_ptr<base::window> window = nullptr;
    std::unordered_map<std::string, std::shared_ptr<base::scene>> scenes = {};
    std::weak_ptr<base::scene> current_scene = {};

    const double target_simulation_time = 1.0 / 60.0;
    double last_simulation_time = 0.0;
    double simulation_accumulator = 0.0;
    double simulation_alpha = 0.0;
    const double target_render_time = 1.0 / 144.0;
    double last_render_time = 0.0;
    double last_fps_time = 0;
    int frame_count = 0;
  };
}

#include "game.inl"
