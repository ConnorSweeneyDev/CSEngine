#include "group.hpp"

#include <span>
#include <string_view>

#include "exception.hpp"

namespace cse::help::group
{
  store::registrar::registrar(const std::span<const std::string_view> tags_) { enlist(tags_); }

  void enlist(const std::span<const std::string_view> tags)
  {
    if (!store.tags.empty())
    {
      store.duplicated = true;
      return;
    }
    store.tags = tags;
  }

  void verify()
  {
    if (store.duplicated) throw exception("Tried to declare GROUPS more than once");
  }
}
