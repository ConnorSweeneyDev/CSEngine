#include "locale.hpp"

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "exception.hpp"

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

  store::registrar::registrar(const std::initializer_list<std::string_view> languages_) { enlist(languages_); }

  store::segment::segment(const char *literal_) : literal{literal_ ? literal_ : ""} {}
  store::segment::segment(const std::string &literal_) : literal{literal_} {}
  store::segment::segment(const std::string_view literal_) : literal{literal_} {}
  store::segment::segment(const locale::key &key_) : pointer{&key_} {}

  store &registry()
  {
    static store instance{};
    return instance;
  }

  void enlist(const std::initializer_list<std::string_view> languages)
  {
    auto &store{registry()};
    if (!store.languages.empty()) throw exception("Tried to declare LANGUAGES more than once");
    if (languages.size() == 0) throw exception("Tried to declare LANGUAGES without any languages");
    for (const auto language : languages)
    {
      for (const auto existing : store.languages)
        if (existing == language) throw exception("Duplicate language '{}' in LANGUAGES", language);
      store.languages.push_back(language);
    }
    store.resolved = false;
  }

  void enlist(const locale::key &key)
  {
    auto &store{registry()};
    for (const auto *existing : store.keys)
      if (existing->label() == key.label()) throw exception("Duplicate translation key '{}'", key.label());
    store.keys.push_back(&key);
    store.resolved = false;
  }

  void resolve(const std::string &language)
  {
    auto &store{registry()};
    if (store.languages.empty())
    {
      if (store.keys.empty()) return;
      throw exception("Translation keys were declared without a LANGUAGES declaration");
    }
    if (language.empty())
      throw exception("The game language is empty but {} language(s) were declared with LANGUAGES",
                      store.languages.size());
    const auto count{store.languages.size()};
    auto target{count};
    for (std::size_t index{}; index < count; ++index)
      if (store.languages.at(index) == language) target = index;
    if (target == count) throw exception("Tried to set the game language to unknown language '{}'", language);

    if (!store.resolved)
    {
      store.table.assign(store.keys.size() * count, {});
      for (const auto *key : store.keys)
      {
        for (const auto &entry : key->entries)
        {
          bool known{false};
          for (const auto language_name : store.languages)
            if (language_name == entry.language) known = true;
          if (!known) throw exception("Translation key '{}' names unknown language '{}'", key->label(), entry.language);
        }
        for (std::size_t index{}; index < count; ++index)
        {
          const auto language_name{store.languages.at(index)};
          std::size_t found{};
          for (const auto &entry : key->entries)
            if (entry.language == language_name)
            {
              store.table.at((key->index * count) + index) = entry.value;
              ++found;
            }
          if (found == 0)
            throw exception("Translation key '{}' is missing a value for language '{}'", key->label(), language_name);
          if (found > 1)
            throw exception("Translation key '{}' has {} values for language '{}'", key->label(), found, language_name);
        }
      }
      store.resolved = true;
    }
    store.current = target;
  }
}

namespace cse
{
  void locale::node::settle()
  {
    constant = true;
    for (const auto &segment : segments)
      if (segment.pointer) constant = false;
    if (!constant) return;
    for (const auto &segment : segments) resolved += segment.literal;
    cached = true;
  }

  locale::locale(const char *literal) : locale(literal ? std::string_view{literal} : std::string_view{}) {}

  locale::locale(const std::string &literal) : locale(std::string_view{literal}) {}

  locale::locale(const std::string_view literal)
  {
    if (literal.empty()) return;
    auto fresh{std::make_shared<node>()};
    fresh->segments.emplace_back(literal);
    fresh->settle();
    handle = std::move(fresh);
  }

  locale::locale(const help::locale::key &key)
  {
    auto fresh{std::make_shared<node>()};
    fresh->segments.emplace_back(key);
    fresh->settle();
    handle = std::move(fresh);
  }

  locale::locale(const std::initializer_list<help::locale::store::segment> segments)
  {
    if (segments.size() == 0) return;
    auto fresh{std::make_shared<node>()};
    fresh->segments.assign(segments.begin(), segments.end());
    fresh->settle();
    handle = std::move(fresh);
  }

  bool locale::operator==(const char *other) const { return string() == (other ? other : ""); }

  bool locale::operator==(const std::string &other) const { return string() == other; }

  bool locale::operator==(const std::string_view other) const { return string() == other; }

  bool locale::operator==(const help::locale::key &other) const { return string() == other.string(); }

  bool locale::operator==(const locale &other) const { return handle == other.handle || string() == other.string(); }

  const std::string &locale::string() const
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
