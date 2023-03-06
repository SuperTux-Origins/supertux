//  SuperTux
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
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

#ifndef HEADER_SUPERTUX_MATH_BEZIER_HPP
#define HEADER_SUPERTUX_MATH_BEZIER_HPP

#include <math/vector.hpp>

class Color;
class DrawingContext;

class Bezier
{
public:
  // p1 is first anchor, p2 is first handle, p3 is second handle, p4 is second anchor. T is progress from p1 towards p4.
  static Vector get_point(Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, float t);
  // Same as above, but does not treat p1 == p2 && p3 == p4 as a special case
  static Vector get_point_raw(Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, float t);
  // Calculates the full length of the bezier curve (approximated)
  static float get_length(Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, int steps = 100);
  // Gets the point at the given length
  static Vector get_point_at_length(Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, float length, int steps = 100);
  // Same as get_point but gets length-normalized
  static Vector get_point_by_length(Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, float t);
  // FIXME: Move this to the Canvas object?
  static void draw_curve(DrawingContext& context, Vector const& p1, Vector const& p2, Vector const& p3, Vector const& p4, int steps, Color color, int layer);

private:
  Bezier(Bezier const&) = delete;
  Bezier& operator=(Bezier const&) = delete;
};

#endif

/* EOF */
