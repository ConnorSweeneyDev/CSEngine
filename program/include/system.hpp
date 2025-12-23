#pragma once

enum platforms
{
  WINDOWS,
  LINUX
};

namespace cse
{
#if defined(_WIN32)
  constexpr platforms platform{WINDOWS};
#elif defined(__linux__)
  constexpr platforms platform{LINUX};
#endif

#if defined(_DEBUG)
  constexpr bool debug{true};
#elif defined(NDEBUG)
  constexpr bool debug{false};
#endif
}
