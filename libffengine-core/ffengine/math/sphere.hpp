/**********************************************************************
File name: sphere.hpp
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
#ifndef SCC_ENGINE_MATH_SPHERE_H
#define SCC_ENGINE_MATH_SPHERE_H

#include "ffengine/math/vector.hpp"


/**
 * An ideal, mathematical sphere.
 */
struct Sphere
{
    /**
     * The center of the sphere.
     */
    Vector3f center;

    /**
     * The radius of the sphere.
     */
    float radius;
};


#endif
