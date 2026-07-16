#pragma once

#include "exception.hpp"
#include "state.hpp"

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#include "log.hpp"

namespace cse
{
  template <typename type> state::field<type>::field(const char *key_, const type &value_) : value{value_}, key{key_}
  {
    building->enlist(
      [this](nlohmann::json &json)
      {
        if (!json.emplace(key, value).second) throw cse::exception("Duplicate state field \"{}\"", key);
      },
      [this](const nlohmann::json &json)
      {
        if (!json.contains(key)) return;
        try
        {
          json.at(key).get_to(value);
        }
        catch (const nlohmann::json::exception &error)
        {
          log("Could not parse state field \"{}\", using default: {}", key, error.what());
        }
      });
  }

  template <typename type> state::field<type>::operator type &() noexcept { return value; }

  template <typename type> state::field<type>::operator const type &() const noexcept { return value; }

  template <typename type> type *state::field<type>::operator->() noexcept { return &value; }

  template <typename type> const type *state::field<type>::operator->() const noexcept { return &value; }

  template <typename type> type &state::field<type>::operator*() noexcept { return value; }

  template <typename type> const type &state::field<type>::operator*() const noexcept { return value; }

  template <typename type> state::field<type> &state::field<type>::operator=(const type &value_)
  {
    value = value_;
    return *this;
  }
}
