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
    friend void resolve(std::string &language);
    template <const auto &entries, const auto &languages> friend key forge(const std::string_view label);

  public:
    struct entry
    {
      std::string_view language{};
      std::string_view value{};
    };

  public:
    ~key() = default;
    key(const key &) = delete;
    key &operator=(const key &) = delete;
    key(key &&) = delete;
    key &operator=(key &&) = delete;

    std::string_view label() const;
    std::string_view string() const;

  private:
    key(const std::string_view label_, const std::span<const entry> entries_);

  private:
    std::string_view identity{};
    std::span<const entry> entries{};
    std::size_t index{};
  };

  constexpr bool distinct(const std::span<const key::entry> entries);
  constexpr bool complete(const std::span<const key::entry> entries, const std::span<const std::string_view> languages);
  template <const auto &entries, const auto &languages> key forge(const std::string_view label);

  struct store
  {
    struct registrar
    {
      registrar(const std::span<const std::string_view> languages_);
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

    std::span<const std::string_view> languages{};
    std::vector<const locale::key *> keys{};
    std::vector<std::string_view> table{};
    std::size_t current{};
    bool resolved{};
    bool duplicated{};
  };

  store &registry();
  void enlist(const std::span<const std::string_view> languages);
  void enlist(const locale::key &key);
  void resolve(std::string &language);
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
#define LANGUAGES(...) CSE_JOIN(CSE_LANGUAGES_, CSE_FILLED(__VA_ARGS__))(__VA_ARGS__)
#define CSE_LANGUAGES_(...) static_assert(false, "LANGUAGES was declared without any languages")
#define CSE_LANGUAGES_FILLED(...)                                                                                      \
  namespace language                                                                                                   \
  {                                                                                                                    \
    CSE_FOR_EACH(CSE_LANGUAGE_DECLARE, __VA_ARGS__)                                                                    \
    namespace detail                                                                                                   \
    {                                                                                                                  \
      inline constexpr std::string_view list[]{__VA_ARGS__};                                                           \
      inline const cse::help::locale::store::registrar languages{list};                                                \
    }                                                                                                                  \
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
    inline const cse::help::locale::key identifier{                                                                    \
      cse::help::locale::forge<detail::identifier, language::detail::list>(#identifier)};                              \
  }                                                                                                                    \
  static_assert(true)

#include "locale.inl" // IWYU pragma: keep
