#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "macro.hpp"

namespace cse::help::locale
{
  class key
  {
    friend void resolve(const std::string &language);

  public:
    struct entry
    {
      std::string_view language{};
      std::string_view value{};
    };

  public:
    key(const std::string_view label_, const std::span<const entry> entries_);
    ~key() = default;
    key(const key &) = delete;
    key &operator=(const key &) = delete;
    key(key &&) = delete;
    key &operator=(key &&) = delete;

    std::string_view label() const;
    std::string_view string() const;

  private:
    std::string_view identity{};
    std::span<const entry> entries{};
    std::size_t index{};
  };

  struct store
  {
    struct registrar
    {
      registrar(const std::initializer_list<std::string_view> languages_);
    };
    struct segment
    {
      segment(const char *literal_);
      segment(const std::string &literal_);
      segment(const std::string_view literal_);
      segment(const locale::key &key_);

      std::string literal{};
      const locale::key *pointer{};
    };

    std::vector<std::string_view> languages{};
    std::vector<const locale::key *> keys{};
    std::vector<std::string_view> table{};
    std::size_t current{};
    bool resolved{};
  };

  store &registry();
  void enlist(const std::initializer_list<std::string_view> languages);
  void enlist(const locale::key &key);
  void resolve(const std::string &language);
}

namespace cse
{
  class lexeme
  {
  private:
    struct node
    {
      void settle();

      std::vector<help::locale::store::segment> segments{};
      bool constant{};
      mutable std::string resolved{};
      mutable std::size_t language{};
      mutable bool cached{};
    };

  public:
    lexeme() = default;
    lexeme(const char *literal);
    lexeme(const std::string &literal);
    lexeme(const std::string_view literal);
    lexeme(const help::locale::key &key);
    lexeme(const std::initializer_list<help::locale::store::segment> segments);
    ~lexeme() = default;
    lexeme(const lexeme &) = default;
    lexeme &operator=(const lexeme &) = default;
    lexeme(lexeme &&) = default;
    lexeme &operator=(lexeme &&) = default;

    bool operator==(const char *other) const;
    bool operator==(const std::string &other) const;
    bool operator==(const std::string_view other) const;
    bool operator==(const help::locale::key &other) const;
    bool operator==(const lexeme &other) const;

    const std::string &string() const;

  private:
    std::shared_ptr<const node> handle{};
  };
}

#define CSE_LANGUAGE_DECLARE(language) inline constexpr const char *language{#language};
#define LANGUAGES(...)                                                                                                 \
  namespace language                                                                                                   \
  {                                                                                                                    \
    CSE_FOR_EACH(CSE_LANGUAGE_DECLARE, __VA_ARGS__)                                                                    \
    namespace detail { inline const cse::help::locale::store::registrar languages{__VA_ARGS__}; }                      \
  }                                                                                                                    \
  static_assert(true)

#define CSE_TRANSLATE_ENTRY(element) CSE_TRANSLATE_ENTRY_ element
#define CSE_TRANSLATE_ENTRY_(name, value) {language::name, value},
#define TRANSLATE(identifier, ...)                                                                                     \
  namespace lexeme                                                                                                     \
  {                                                                                                                    \
    namespace detail                                                                                                   \
    {                                                                                                                  \
      inline constexpr cse::help::locale::key::entry identifier[]{CSE_FOR_EACH(CSE_TRANSLATE_ENTRY, __VA_ARGS__)};     \
    }                                                                                                                  \
    inline const cse::help::locale::key identifier{#identifier, detail::identifier};                                   \
  }                                                                                                                    \
  static_assert(true)
