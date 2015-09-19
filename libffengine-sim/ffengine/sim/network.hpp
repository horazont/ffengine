/**********************************************************************
File name: network.hpp
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
#ifndef SCC_SIM_NETWORK_H
#define SCC_SIM_NETWORK_H

// NOTE: This file has nothing to do with computer networks. It deals with the
// simulation of the transport network!

#include <ostream>

#include "ffengine/math/vector.hpp"

#include "ffengine/sim/objects.hpp"


namespace sim {


struct PhysicalEdgeSegment
{
    PhysicalEdgeSegment(const float s0,
                        const Vector3f &start,
                        const Vector3f &direction);

    float s0;
    Vector3f start;
    Vector3f direction;

    bool operator==(const PhysicalEdgeSegment &other) const;

    inline bool operator!=(const PhysicalEdgeSegment &other) const
    {
        return !((*this) == other);
    }

};


void offset_segments(const std::vector<PhysicalEdgeSegment> &segments,
                     const float offset,
                     const Vector3f &exit_direction,
                     std::vector<PhysicalEdgeSegment> &dest);


std::ostream &operator<<(std::ostream &dest, const PhysicalEdgeSegment &segment);

}

#endif
