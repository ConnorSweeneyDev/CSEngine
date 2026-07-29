#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "nlohmann/json_fwd.hpp"

#include "core.hpp"
#include "macro.hpp"
#include "name.hpp"

namespace cse
{
  class state
  {
    friend class game;

  protected:
    struct initial
    {
      const std::filesystem::path storage{};
    };
    template <typename type> class field
    {
    public:
      explicit field(const char *key_, const type &value_ = {});
      ~field() = default;
      field(const field &) = delete;
      field &operator=(const field &) = delete;
      field(field &&) = delete;
      field &operator=(field &&) = delete;

      explicit operator type &() noexcept;
      explicit operator const type &() const noexcept;
      type *operator->() noexcept;
      const type *operator->() const noexcept;
      type &operator*() noexcept;
      const type &operator*() const noexcept;
      field &operator=(const type &value_);

    public:
      type value{};

    private:
      std::string key{};
    };

  private:
    struct entry
    {
      std::function<void(nlohmann::json &json)> writer{};
      std::function<void(const nlohmann::json &json)> reader{};
    };

  public:
    virtual ~state() = default;
    state(const state &) = delete;
    state &operator=(const state &) = delete;
    state(state &&) = delete;
    state &operator=(state &&) = delete;

    bool read();
    bool write() const;

  protected:
    explicit state(const initial &initial_);

  private:
    void enlist(std::function<void(nlohmann::json &json)> writer,
                std::function<void(const nlohmann::json &json)> reader);

  public:
    cse::identity name{};
    std::filesystem::path storage{};

  private:
    std::vector<entry> entries{};
    inline static thread_local state *building{};
  };
}

#define CSE_ENLIST_DECLARE(element) CSE_ENLIST_DECLARE_ element
#define CSE_ENLIST_DECLARE_(name, type, ...) CSE_DEPAREN(type) name __VA_ARGS__;
#define CSE_ENLIST_WRITE(element) CSE_ENLIST_WRITE_ element
#define CSE_ENLIST_WRITE_(name, type, ...) json[#name] = value.name;
#define CSE_ENLIST_READ(element) CSE_ENLIST_READ_ element
#define CSE_ENLIST_READ_(name, type, ...)                                                                              \
  if (json.contains(#name)) json.at(#name).get_to(value.name);
#define ENLIST(identifier, ...)                                                                                        \
  struct identifier                                                                                                    \
  {                                                                                                                    \
    CSE_FOR_EACH(CSE_ENLIST_DECLARE, __VA_ARGS__)                                                                      \
    friend void to_json(nlohmann::json &json, const identifier &value) { CSE_FOR_EACH(CSE_ENLIST_WRITE, __VA_ARGS__) } \
    friend void from_json(const nlohmann::json &json, identifier &value)                                               \
    { CSE_FOR_EACH(CSE_ENLIST_READ, __VA_ARGS__) }                                                                     \
  }

#define STORE(identifier, type, ...)                                                                                   \
  state::field<CSE_DEPAREN(type)> identifier { #identifier, __VA_ARGS__ }

#include "state.inl" // IWYU pragma: keep
