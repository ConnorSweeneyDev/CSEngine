#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "nlohmann/json_fwd.hpp"

#include "core.hpp"
#include "macro.hpp"
#include "name.hpp"

namespace cse::help
{
  namespace state { void log(std::string_view reason); }

  inline thread_local std::string marker_trail{};

  class marker
  {
  public:
    explicit marker(const std::string_view name_);
    ~marker();
    marker(const marker &) = delete;
    marker &operator=(const marker &) = delete;
    marker(marker &&) = delete;
    marker &operator=(marker &&) = delete;

  private:
    std::size_t length{};
  };
}

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

  public:
    virtual ~state() = default;
    state(const state &) = delete;
    state &operator=(const state &) = delete;
    state(state &&) = delete;
    state &operator=(state &&) = delete;

    bool read();
    bool write();

  protected:
    explicit state(const initial &initial_);

    virtual void enroll() = 0;
    template <typename type> void enlist(const char *key, type &value);

  public:
    cse::identity name{};
    std::filesystem::path storage{};

  private:
    nlohmann::json *document{};
    bool writing{};
  };
}

#define CSE_ENLIST_DECLARE(element) CSE_ENLIST_DECLARE_ element
#define CSE_ENLIST_DECLARE_(name, type, ...)                                                                           \
  CSE_JOIN(CSE_ENLIST_VALUE_, CSE_FILLED(__VA_ARGS__))(name, type, __VA_ARGS__)
#define CSE_ENLIST_VALUE_(name, type, ...)                                                                             \
  CSE_DEPAREN(type) name{};                                                                                            \
  static_assert(false, "ENLIST field \"" #name "\" was declared without an initializer");
#define CSE_ENLIST_VALUE_FILLED(name, type, ...) CSE_DEPAREN(type) name __VA_ARGS__;
#define CSE_ENLIST_WRITE(element) CSE_ENLIST_WRITE_ element
#define CSE_ENLIST_WRITE_(name, type, ...) json[#name] = value.name;
#define CSE_ENLIST_READ(element) CSE_ENLIST_READ_ element
#define CSE_ENLIST_READ_(name, type, ...)                                                                              \
  if (json.contains(#name))                                                                                            \
  {                                                                                                                    \
    const cse::help::marker marker{#name};                                                                             \
    try                                                                                                                \
    {                                                                                                                  \
      json.at(#name).get_to(value.name);                                                                               \
    }                                                                                                                  \
    catch (const nlohmann::json::exception &error)                                                                     \
    {                                                                                                                  \
      cse::help::state::log(std::format(": {}", error.what()));                                                        \
    }                                                                                                                  \
  }
#define ENLIST(identifier, ...)                                                                                        \
private:                                                                                                               \
  struct identifier                                                                                                    \
  {                                                                                                                    \
    CSE_FOR_EACH(CSE_ENLIST_DECLARE, __VA_ARGS__)                                                                      \
    friend void to_json(nlohmann::json &json, const identifier &value) { CSE_FOR_EACH(CSE_ENLIST_WRITE, __VA_ARGS__) } \
    friend void from_json(const nlohmann::json &json, identifier &value)                                               \
    {                                                                                                                  \
      if (!json.is_object())                                                                                           \
      {                                                                                                                \
        cse::help::state::log(std::format("s: type must be object, but is {}", json.type_name()));                     \
        return;                                                                                                        \
      }                                                                                                                \
      CSE_FOR_EACH(CSE_ENLIST_READ, __VA_ARGS__)                                                                       \
    }                                                                                                                  \
  }

#define CSE_STORE_DECLARE(element) CSE_STORE_DECLARE_ element
#define CSE_STORE_DECLARE_(name, type, ...) CSE_JOIN(CSE_STORE_VALUE_, CSE_FILLED(__VA_ARGS__))(name, type, __VA_ARGS__)
#define CSE_STORE_VALUE_(name, type, ...)                                                                              \
  CSE_DEPAREN(type) name{};                                                                                            \
  static_assert(false, "STORE field \"" #name "\" was declared without an initializer");
#define CSE_STORE_VALUE_FILLED(name, type, ...) CSE_DEPAREN(type) name __VA_ARGS__;
#define CSE_STORE_ENROLL(element) CSE_STORE_ENROLL_ element
#define CSE_STORE_ENROLL_(name, type, ...) enlist(#name, name);
#define STORE(...)                                                                                                     \
private:                                                                                                               \
  void enroll() final { CSE_FOR_EACH(CSE_STORE_ENROLL, __VA_ARGS__) }                                                  \
                                                                                                                       \
public:                                                                                                                \
  CSE_FOR_EACH(CSE_STORE_DECLARE, __VA_ARGS__)                                                                         \
  static_assert(true)

#include "state.inl" // IWYU pragma: keep
