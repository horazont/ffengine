/**********************************************************************
File name: tikz.hpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
#ifndef SCC_MATH_TIKZ_H
#define SCC_MATH_TIKZ_H

#include <ostream>

template <typename float_t, unsigned int n>
struct Vector;

typedef Vector<float, 2> Vector2f;

struct Line2f;


void tikz_draw(std::ostream &dest,
               const Vector2f &origin, const Vector2f &direction,
               const std::string &draw_flags = "",
               const std::string &label_text = "",
               const std::string &label_flags = "");
void tikz_draw_line_around_origin(std::ostream &dest,
                                  const Vector2f &origin,
                                  const Vector2f &direction,
                                  const float partition = 0.5f,
                                  const std::string &draw_flags = "",
                                  const std::string &label_text = "",
                                  const std::string &label_flags = "");
void tikz_node(std::ostream &dest,
               const Vector2f &origin,
               const std::string &node_text = "",
               const std::string &node_flags = "");


#endif
