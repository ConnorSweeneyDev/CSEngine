#pragma once

#include "collision.hpp"

namespace cse::help
{
  template <typename callable> void collision::handle(callable &&config) const
  {
    for (const auto &contact : contacts) config(contact);
  }
}
