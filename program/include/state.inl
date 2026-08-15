#pragma once

#include "state.hpp"

#include <exception>

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#include "exception.hpp"
#include "log.hpp"

namespace cse
{
  template <typename type> void state::enlist(const char *key, type &value)
  {
    if (!document) throw cse::exception("Tried to enlist state field \"{}\" outside of a read or write", key);
    if (writing)
    {
      if (!document->emplace(key, value).second) throw cse::exception("Duplicate state field \"{}\"", key);
      return;
    }
    if (!document->contains(key)) return;
    const help::marker marker{key};
    try
    {
      document->at(key).get_to(value);
    }
    catch (const std::exception &error)
    {
      fallback = true;
      log("Could not parse state field \"{}\": {}", help::marker_trail, error.what());
    }
  }
}
