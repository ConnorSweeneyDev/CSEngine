#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "declaration.hpp"
#include "graphics.hpp"
#include "hook.hpp"
#include "id.hpp"
#include "previous.hpp"
#include "state.hpp"
#include "traits.hpp"

namespace cse
{
  class game : public std::enable_shared_from_this<game>
  {
  public:
    virtual ~game();
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <help::is_window window_type, typename... window_arguments>
    std::shared_ptr<game> set_window(window_arguments &&...arguments);
    template <help::is_window window_type, typename... window_arguments> std::shared_ptr<game>
    set_window(const std::function<void(const std::shared_ptr<window_type>)> &config, window_arguments &&...arguments);
    template <help::is_callable callable, typename... window_arguments>
    std::shared_ptr<game> set_window(callable &&config, window_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_scene(const help::id name, scene_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_scene(const help::id name,
                                    const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                    scene_arguments &&...arguments);
    template <help::is_callable callable, typename... scene_arguments>
    std::shared_ptr<game> set_scene(const help::id name, callable &&config, scene_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_current_scene(const help::id name, scene_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_current_scene(const help::id name,
                                            const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                            scene_arguments &&...arguments);
    template <help::is_callable callable, typename... scene_arguments>
    std::shared_ptr<game> set_current_scene(const help::id name, callable &&config, scene_arguments &&...arguments);
    std::shared_ptr<game> set_current_scene(const help::id name);
    std::shared_ptr<game> remove_scene(const help::id name);

    template <help::is_game game_type, typename... game_arguments> static std::shared_ptr<game_type>
    create(const std::function<void(const std::shared_ptr<game_type>)> &config, game_arguments &&...arguments);
    template <help::is_callable callable, typename... game_arguments>
    static std::shared_ptr<game> create(callable &&config, game_arguments &&...arguments);
    void run();

  protected:
    game(const std::pair<double, double> &rates_);

  private:
    void initialize();
    void event();
    void input();
    void simulate();
    void render();
    void cleanup();

    void update_parents();
    void process_updates();
    void update_previous();
    void update_time();
    bool simulation_behind();
    bool should_render();
    void update_fps();

  public:
    std::shared_ptr<class window> window{};
    std::unordered_map<help::id, std::shared_ptr<class scene>> scenes{};
    help::game_state state{};
    help::game_graphics graphics{};
    help::game_previous previous{};
    help::hook hook{};

  private:
    static inline std::weak_ptr<game> instance{};
    static constexpr float scale_factor{1.0f / 25.0f};
    static constexpr float aspect_ratio{16.0f / 9.0f};
    double active_poll_rate{1.0 / state.poll_rate};
    double active_frame_rate{1.0 / graphics.frame_rate};
    double time{};
    double accumulator{};
    double alpha{};
  };
}

#include "game.inl" // IWYU pragma: keep
