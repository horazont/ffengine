/**********************************************************************
File name: tikz.cpp
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
#include "ffengine/math/tikz.hpp"

#include "ffengine/math/vector.hpp"



void tikz_draw(std::ostream &dest, const Vector2f &origin, const Vector2f &direction, const std::string &draw_flags, const std::string &label_text, const std::string &label_flags)
{
    dest << "\\draw[" << draw_flags << "] "
         << "(" << origin[eX] << ", " << origin[eY] << ") -- "
         << "++(" << direction[eX] << ", " << direction[eY] << ")";

    if (!label_text.empty()) {
        dest << " node[" << label_flags << "] {" << label_text << "}";
    }

    dest << ";" << std::endl;
}


void tikz_node(std::ostream &dest,
               const Vector2f &origin,
               const std::string &node_text,
               const std::string &node_flags)
{
    dest << "\\node[" << node_flags << "] at (" << origin[eX] << ", " << origin[eY] << ") "
         << "{" << node_text << "};" << std::endl;
}


void tikz_draw_line_around_origin(std::ostream &dest,
                                  const Vector2f &origin,
                                  const Vector2f &direction,
                                  const float partition,
                                  const std::string &draw_flags,
                                  const std::string &label_text,
                                  const std::string &label_flags)
{
    tikz_draw(dest, origin - direction*partition, direction, draw_flags, label_text, label_flags);
}
