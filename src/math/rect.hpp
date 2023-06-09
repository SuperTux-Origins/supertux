//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_MATH_RECT_HPP
#define HEADER_SUPERTUX_MATH_RECT_HPP

#include <iosfwd>

#include <SDL.h>
#include <algorithm>
#include <tuple>

#include "math/size.hpp"

class Rectf;

class Rect final
{
public:
  int left;
  int top;
  int right;
  int bottom;

public:
  static Rect from_center(int center_x, int center_y, int width, int height)
  {
    return Rect(center_x - width / 2,
                center_y - height / 2,
                center_x + width / 2,
                center_y + height / 2);
  }

public:
  Rect() :
    left(0),
    top(0),
    right(0),
    bottom(0)
  {}

  Rect(Rect const& rhs) = default;
  Rect& operator=(Rect const& rhs) = default;

  Rect(int left_, int top_, int right_, int bottom_) :
    left(left_),
    top(top_),
    right(right_),
    bottom(bottom_)
  {}

  Rect(int left_, int top_, Size const& size) :
    left(left_),
    top(top_),
    right(left_ + size.width),
    bottom(top_ + size.height)
  {}

  Rect(SDL_Rect const& rect) :
    left(rect.x),
    top(rect.y),
    right(rect.x + rect.w),
    bottom(rect.y + rect.h)
  {}

  explicit Rect(Rectf const& other);

  bool operator==(Rect const& other) const
  {
    return (left == other.left &&
            top == other.top &&
            right == other.right &&
            bottom == other.bottom);
  }

  bool contains(int x, int y) const
  {
    return (left <= x && x < right &&
            top <= y && y < bottom);
  }

  bool contains(Rect const& other) const
  {
    return (left <= other.left && other.right <= right &&
            top <= other.top && other.bottom <= bottom);
  }

  int get_width()  const { return right - left; }
  int get_height() const { return bottom - top; }
  Size get_size() const { return Size(right - left, bottom - top); }
  int get_area() const { return get_width() * get_height(); }

  bool empty() const
  {
    return (get_width() <= 0 ||
            get_height() <= 0);
  }

  bool valid() const
  {
    return left <= right && top <= bottom;
  }

  Rect normalized() const
  {
    return Rect(std::min(left, right),
                std::min(top, bottom),
                std::max(left, right),
                std::max(top, bottom));
  }

  Rect moved(int x, int y) const
  {
    return Rect(left + x,
                top + y,
                right + x,
                bottom + y);
  }

  Rect grown(int border) const
  {
    return Rect(left - border,
                top - border,
                right + border,
                bottom + border);
  }

  SDL_Rect to_sdl() const
  {
    return {left, top, get_width(), get_height()};
  }

  bool operator<(Rect const& other) const {
#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
    return std::tie(left, top, right, bottom) < std::tie(other.left, other.top, other.right, other.bottom); // NOLINT
  }
};

std::ostream& operator<<(std::ostream& out, Rect const& rect);

#endif

/* EOF */
