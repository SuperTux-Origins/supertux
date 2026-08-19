// SPDX-FileCopyrightText: 1999–2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// C++23 std::print / std::println when available; otherwise std::format + fwrite
// polyfill (R36S / older libstdc++, etc.). Adapted from Pingus util/print.hpp.

#ifndef HEADER_SUPERTUX_UTIL_PRINT_HPP
#define HEADER_SUPERTUX_UTIL_PRINT_HPP

#include <cstdio>
#include <format>
#include <string>
#include <utility>

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L && !defined(_WIN32)
#  include <print>
#  define SUPERTUX_HAS_STD_PRINT 1
#else
#  define SUPERTUX_HAS_STD_PRINT 0
#endif

namespace supertux {

#if SUPERTUX_HAS_STD_PRINT

template<class... Args>
void print(std::FILE* stream, std::format_string<Args...> fmt, Args&&... args)
{
  std::print(stream, fmt, std::forward<Args>(args)...);
}

template<class... Args>
void println(std::FILE* stream, std::format_string<Args...> fmt, Args&&... args)
{
  std::println(stream, fmt, std::forward<Args>(args)...);
}

template<class... Args>
void println(std::format_string<Args...> fmt, Args&&... args)
{
  std::println(fmt, std::forward<Args>(args)...);
}

template<class... Args>
void print_err(std::format_string<Args...> fmt, Args&&... args)
{
  std::println(stderr, fmt, std::forward<Args>(args)...);
  fflush(stderr);
}

#else // polyfill

template<class... Args>
void print(std::FILE* stream, std::format_string<Args...> fmt, Args&&... args)
{
  auto s = std::format(fmt, std::forward<Args>(args)...);
  fwrite(s.data(), 1, s.size(), stream);
}

template<class... Args>
void println(std::FILE* stream, std::format_string<Args...> fmt, Args&&... args)
{
  auto s = std::format(fmt, std::forward<Args>(args)...);
  s.push_back('\n');
  fwrite(s.data(), 1, s.size(), stream);
}

template<class... Args>
void println(std::format_string<Args...> fmt, Args&&... args)
{
  println(stdout, fmt, std::forward<Args>(args)...);
}

template<class... Args>
void print_err(std::format_string<Args...> fmt, Args&&... args)
{
  auto s = std::format(fmt, std::forward<Args>(args)...);
  s.push_back('\n');
  fwrite(s.data(), 1, s.size(), stderr);
  fflush(stderr);
}

#endif

} // namespace supertux

#endif

/* EOF */
