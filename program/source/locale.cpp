#include "locale.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "exception.hpp"
#include "log.hpp"

namespace cse::help::locale
{
  key::key(const std::string_view label_, const std::span<const entry> entries_)
    : identity{label_}, entries{entries_}, index{registry().keys.size()}
  { enlist(*this); }

  std::string_view key::label() const { return identity; }

  std::string_view key::string() const
  {
    const auto &store{registry()};
    if (!store.resolved) throw exception("Tried to read translation key '{}' before the language was set", identity);
    return store.table.at((index * store.languages.size()) + store.current);
  }

  store::registrar::registrar(const std::span<const std::string_view> languages_) { enlist(languages_); }

  store::segment::segment(const char *literal_) : literal{literal_ ? literal_ : ""} {}

  store::segment::segment(const std::string &literal_) : literal{literal_} {}

  store::segment::segment(const std::string_view literal_) : literal{literal_} {}

  store::segment::segment(const locale::key &key_) : pointer{&key_} {}

  store &registry()
  {
    static store instance{};
    return instance;
  }

  void enlist(const std::span<const std::string_view> languages)
  {
    auto &store{registry()};
    if (!store.languages.empty())
    {
      store.duplicated = true;
      return;
    }
    store.languages = languages;
    store.resolved = false;
  }

  void enlist(const locale::key &key)
  {
    auto &store{registry()};
    store.keys.push_back(&key);
    store.resolved = false;
  }

  void resolve(std::string &language)
  {
    auto &store{registry()};
    if (store.languages.empty() && store.keys.empty()) return;
    if (store.duplicated) throw exception("Tried to declare LANGUAGES more than once");
    const auto fallback{store.languages.front()};
    if (language.empty())
    {
      log("The game language is empty; falling back to '{}'", fallback);
      language = fallback;
    }
    auto target{std::ranges::find(store.languages, std::string_view{language})};
    if (target == store.languages.end())
    {
      log("Tried to set the game language to unknown language '{}'; falling back to '{}'", language, fallback);
      language = fallback;
      target = store.languages.begin();
    }
    const auto count{store.languages.size()};

    if (!store.resolved)
    {
      store.table.assign(store.keys.size() * count, {});
      for (const auto *key : store.keys)
      {
        std::size_t index{};
        for (const auto language_name : store.languages)
        {
          store.table.at((key->index * count) + index) =
            std::ranges::find(key->entries, language_name, &locale::key::entry::language)->value;
          ++index;
        }
      }
      store.resolved = true;
    }
    store.current = static_cast<std::size_t>(target - store.languages.begin());
  }
}

namespace cse
{
  void lexeme::node::settle()
  {
    constant = true;
    for (const auto &segment : segments)
      if (segment.pointer) constant = false;
    if (!constant) return;
    for (const auto &segment : segments) resolved += segment.literal;
    cached = true;
  }

  lexeme::lexeme(const char *literal) : lexeme(literal ? std::string_view{literal} : std::string_view{}) {}

  lexeme::lexeme(const std::string &literal) : lexeme(std::string_view{literal}) {}

  lexeme::lexeme(const std::string_view literal)
  {
    if (literal.empty()) return;
    auto fresh{std::make_shared<node>()};
    fresh->segments.emplace_back(literal);
    fresh->settle();
    handle = std::move(fresh);
  }

  lexeme::lexeme(const help::locale::key &key)
  {
    auto fresh{std::make_shared<node>()};
    fresh->segments.emplace_back(key);
    fresh->settle();
    handle = std::move(fresh);
  }

  lexeme::lexeme(const std::initializer_list<help::locale::store::segment> segments)
  {
    if (segments.size() == 0) return;
    auto fresh{std::make_shared<node>()};
    fresh->segments.assign(segments.begin(), segments.end());
    fresh->settle();
    handle = std::move(fresh);
  }

  bool lexeme::operator==(const char *other) const { return string() == (other ? other : ""); }

  bool lexeme::operator==(const std::string &other) const { return string() == other; }

  bool lexeme::operator==(const std::string_view other) const { return string() == other; }

  bool lexeme::operator==(const help::locale::key &other) const { return string() == other.string(); }

  bool lexeme::operator==(const lexeme &other) const { return handle == other.handle || string() == other.string(); }

  const std::string &lexeme::string() const
  {
    static const std::string blank{};
    if (!handle) return blank;
    const auto &target{*handle};
    const auto current{help::locale::registry().current};
    if (target.cached && (target.constant || target.language == current)) return target.resolved;
    target.resolved.clear();
    for (const auto &segment : target.segments)
      if (segment.pointer)
        target.resolved += segment.pointer->string();
      else
        target.resolved += segment.literal;
    target.language = current;
    target.cached = true;
    return target.resolved;
  }
}
