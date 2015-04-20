/**********************************************************************
File name: perlin.hpp
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
#ifndef SCC_ENGINE_MATH_PERLIN_H
#define SCC_ENGINE_MATH_PERLIN_H

#include "engine/math/vector.hpp"


class PerlinNoiseGenerator
{
public:
    PerlinNoiseGenerator(
            const Vector3 &offset, const Vector3 &scale,
            const VectorFloat persistence,
            const unsigned int octaves,
            const VectorFloat largest_feature);
private:
    Vector3 m_offset;
    Vector3 m_scale;
    VectorFloat m_persistence;
    unsigned int m_octaves;
    VectorFloat m_base_frequency;
public:
    VectorFloat get(const Vector2 &pos) const;

};

VectorFloat perlin_rng(const int x, const int y);
VectorFloat perlin_rng_interpolated(const Vector2 &pos);


#endif
