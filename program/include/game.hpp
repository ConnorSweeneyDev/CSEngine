#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "scene.hpp"
#include "window.hpp"

namespace cse::base
{
  class game
  {
  public:
    game(std::unique_ptr<window> custom_window);
    ~game();

    void add_scene(const std::string &name, std::unique_ptr<scene> custom_scene);
    void set_current_scene(const std::string &name);
    void run();

  private:
    void initialize();
    void cleanup();
    bool is_running();
    void input();
    void simulate();
    void render();

    void update_simulation_time();
    bool simulation_behind();
    void update_simulation_alpha();
    bool render_behind();
    void update_fps();

  private:
    std::unique_ptr<window> window;
    std::unordered_map<std::string, std::unique_ptr<scene>> scenes = {};
    scene *current_scene = nullptr;

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
