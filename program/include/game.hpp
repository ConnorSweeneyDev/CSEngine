#pragma once

#include <functional>
#include <memory>

#include "declaration.hpp"
#include "graphics.hpp"
#include "hooks.hpp"
#include "name.hpp"
#include "state.hpp"
#include "traits.hpp"
#include "wrapper.hpp"

namespace cse
{
  class game : public std::enable_shared_from_this<game>
  {
  protected:
    struct hook : public enumeration<hook>
    {
      using enumeration::enumeration;
      inline static const enumeration_value<hook> PRE_PREPARE{};
      inline static const enumeration_value<hook> POST_PREPARE{};
      inline static const enumeration_value<hook> PRE_CREATE{};
      inline static const enumeration_value<hook> POST_CREATE{};
      inline static const enumeration_value<hook> PRE_SYNC{};
      inline static const enumeration_value<hook> POST_SYNC{};
      inline static const enumeration_value<hook> PRE_EVENT{};
      inline static const enumeration_value<hook> POST_EVENT{};
      inline static const enumeration_value<hook> PRE_INPUT{};
      inline static const enumeration_value<hook> POST_INPUT{};
      inline static const enumeration_value<hook> PRE_SIMULATE{};
      inline static const enumeration_value<hook> POST_SIMULATE{};
      inline static const enumeration_value<hook> PRE_RENDER{};
      inline static const enumeration_value<hook> POST_RENDER{};
      inline static const enumeration_value<hook> PRE_DESTROY{};
      inline static const enumeration_value<hook> POST_DESTROY{};
      inline static const enumeration_value<hook> PRE_CLEAN{};
      inline static const enumeration_value<hook> POST_CLEAN{};
    };

  public:
    virtual ~game();
    game(const game &) = delete;
    game &operator=(const game &) = delete;
    game(game &&) = delete;
    game &operator=(game &&) = delete;

    template <help::is_game game_type, typename... game_arguments> static std::shared_ptr<game_type>
    create(const std::function<void(const std::shared_ptr<game_type>)> &config, game_arguments &&...arguments);
    template <help::is_callable callable, typename... game_arguments>
    static std::shared_ptr<game> create(callable &&config, game_arguments &&...arguments);
    template <help::is_window window_type, typename... window_arguments>
    std::shared_ptr<game> set_window(window_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_scene(const help::name name,
                                    const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                    scene_arguments &&...arguments);
    template <help::is_callable callable, typename... scene_arguments>
    std::shared_ptr<game> set_scene(const help::name name, callable &&config, scene_arguments &&...arguments);
    template <help::is_scene scene_type, typename... scene_arguments>
    std::shared_ptr<game> set_current_scene(const help::name name,
                                            const std::function<void(const std::shared_ptr<scene_type>)> &config,
                                            scene_arguments &&...arguments);
    template <help::is_callable callable, typename... scene_arguments>
    std::shared_ptr<game> set_current_scene(const help::name name, callable &&config, scene_arguments &&...arguments);
    std::shared_ptr<game> set_current_scene(const help::name name);
    std::shared_ptr<game> remove_scene(const help::name name);

    void run();

  protected:
    game(const double poll_rate_, const double frame_rate, const double aspect_ratio_);

  private:
    void prepare();
    void create();
    void previous();
    void sync();
    void event();
    void input();
    void simulate();
    void render();
    void destroy();
    void clean();

    bool running();
    void time();
    bool behind();
    bool ready();
    void fps();

  public:
    help::game_state state{};
    help::game_graphics graphics{};
    help::hooks hooks{};

  private:
    static inline std::weak_ptr<game> instance{};
  };
}

#include "game.inl" // IWYU pragma: keep
