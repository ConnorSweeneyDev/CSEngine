#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "nlohmann/json_fwd.hpp"

#include "core.hpp"
#include "name.hpp"

namespace cse
{
  class state
  {
    friend class game;

  protected:
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

    void read();
    void write() const;

  protected:
    explicit state(std::filesystem::path location_);

  private:
    void enlist(std::function<void(nlohmann::json &json)> writer,
                std::function<void(const nlohmann::json &json)> reader);
    std::filesystem::path directory() const;

  public:
    cse::name name{};
    std::filesystem::path location{};

  private:
    std::vector<entry> entries{};
    inline static thread_local state *building{};
  };
}

#define CSE_DEPAREN(type) CSE_DEPAREN_ESCAPE(CSE_DEPAREN_ISH type)
#define CSE_DEPAREN_ISH(...) CSE_DEPAREN_ISH __VA_ARGS__
#define CSE_DEPAREN_ESCAPE(...) CSE_DEPAREN_ESCAPE_(__VA_ARGS__)
#define CSE_DEPAREN_ESCAPE_(...) CSE_DEPAREN_VANISH##__VA_ARGS__
#define CSE_DEPAREN_VANISHCSE_DEPAREN_ISH

#define CSE_PARENS ()
#define CSE_EXPAND(...) CSE_EXPAND3(CSE_EXPAND3(CSE_EXPAND3(CSE_EXPAND3(__VA_ARGS__))))
#define CSE_EXPAND3(...) CSE_EXPAND2(CSE_EXPAND2(CSE_EXPAND2(CSE_EXPAND2(__VA_ARGS__))))
#define CSE_EXPAND2(...) CSE_EXPAND1(CSE_EXPAND1(CSE_EXPAND1(CSE_EXPAND1(__VA_ARGS__))))
#define CSE_EXPAND1(...) __VA_ARGS__
#define CSE_FOR_EACH(op, ...) __VA_OPT__(CSE_EXPAND(CSE_FOR_EACH_(op, __VA_ARGS__)))
#define CSE_FOR_EACH_(op, a, ...) op(a) __VA_OPT__(CSE_FOR_EACH_AGAIN CSE_PARENS(op, __VA_ARGS__))
#define CSE_FOR_EACH_AGAIN() CSE_FOR_EACH_

#define CSE_ENLIST_DECLARE(elem) CSE_ENLIST_DECLARE_ elem
#define CSE_ENLIST_DECLARE_(name, type, ...) CSE_DEPAREN(type) name __VA_ARGS__;
#define CSE_ENLIST_WRITE(elem) CSE_ENLIST_WRITE_ elem
#define CSE_ENLIST_WRITE_(name, type, ...) json[#name] = value.name;
#define CSE_ENLIST_READ(elem) CSE_ENLIST_READ_ elem
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

#define FIELD(identifier, type, ...)                                                                                   \
  state::field<CSE_DEPAREN(type)> identifier { #identifier, __VA_ARGS__ }

#include "state.inl" // IWYU pragma: keep
