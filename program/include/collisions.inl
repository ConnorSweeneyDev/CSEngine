#pragma once

#include "collisions.hpp"

#include "hitbox.hpp"
#include "name.hpp"

namespace cse::help
{
  template <typename callable> void collisions::handle(callable &&config) const
  {
    for (const auto &[target, entries] : contacts)
      for (const auto &entry : entries) config(entry);
  }

  template <typename callable> void collisions::handle(const name target, callable &&config) const
  {
    if (auto iterator{contacts.find(target)}; iterator != contacts.end())
      for (const auto &entry : iterator->second) config(entry);
  }

  template <typename callable> void collisions::handle(const name target, const hitbox own, callable &&config) const
  {
    if (auto iterator{contacts.find(target)}; iterator != contacts.end())
      for (const auto &entry : iterator->second)
        if (entry.self.hitbox == own) config(entry);
  }

  template <typename callable>
  void collisions::handle(const name target, const hitbox own, const hitbox theirs, callable &&config) const
  {
    if (auto iterator{contacts.find(target)}; iterator != contacts.end())
      for (const auto &entry : iterator->second)
        if (entry.self.hitbox == own && entry.target.hitbox == theirs) config(entry);
  }
}
