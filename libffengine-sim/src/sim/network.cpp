/**********************************************************************
File name: network.cpp
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
#include "ffengine/sim/network.hpp"

#include "ffengine/math/curve.hpp"


namespace sim {

/* sim::PhysicalEdgeSegment */

PhysicalEdgeSegment::PhysicalEdgeSegment(const float s0,
                                         const Vector3f &start,
                                         const Vector3f &direction):
    s0(s0),
    start(start),
    direction(direction)
{

}

bool PhysicalEdgeSegment::operator==(const PhysicalEdgeSegment &other) const
{
    return (s0 == other.s0 &&
            start == other.start &&
            direction == other.direction);
}


void offset_segments(const std::vector<PhysicalEdgeSegment> &segments,
                     const float offset,
                     const Vector3f &exit_direction,
                     std::vector<PhysicalEdgeSegment> &dest)
{
    if (segments.empty()) {
        return;
    }

    const Vector3f up(0, 0, 1);
    float s = segments[0].s0;
    Vector3f prev_end(segments[0].start + (segments[0].direction.normalized() % up) * offset);

    for (unsigned int i = 0; i < segments.size(); ++i)
    {
        const PhysicalEdgeSegment &segment = segments[i];

        const Vector3f start(prev_end);
        const Vector3f bitangent(segment.direction.normalized() % up);

        Vector3f curr_bitangent(bitangent);
        if (i == segments.size() - 1) {
            curr_bitangent += (exit_direction % up).normalized();
        } else {
            curr_bitangent += (segments[i+1].direction.normalized() % up).normalized();
        }
        curr_bitangent.normalize();

        curr_bitangent = curr_bitangent / (curr_bitangent * bitangent) * offset;

        const Vector3f end(segment.start + segment.direction + curr_bitangent);

        prev_end = end;

        const Vector3f direction(end - start);
        dest.emplace_back(s, start, direction);
        s += direction.length();
    }
}


std::ostream &operator<<(std::ostream &dest, const PhysicalEdgeSegment &segment)
{
    return dest << "PhysicalEdgeSegment("
         << segment.s0
         << ", " << segment.start
         << ", " << segment.direction
         << ")";
}


void segmentize_curve(const QuadBezier3f &curve,
                      std::vector<QuadBezier3f> &segments)
{
    const float segment_length = 10;
    const float min_length = 5;
    std::vector<float> ts;
    // we use the autosampled points as a reference for where we can approximate
    // the curve using line segments. inside those segments, we split as
    // neccessary
    autosample_quadbezier(curve, std::back_inserter(ts));

    std::vector<float> segment_ts;

    float len_accum = 0.f;
    float prev_t = 0.f;
    Vector3f prev_point = curve[0.];
    for (float sampled_t: ts) {
        Vector3f curr_point = curve[sampled_t];
        float segment_len = (prev_point - curr_point).length();
        float existing_len = len_accum;
        float split_t = 0.f;
        len_accum += segment_len;

        if (len_accum >= segment_length) {
            // special handling for re-using the existing length
            float local_len = segment_length - existing_len;
            split_t = prev_t + (sampled_t - prev_t) * local_len / segment_len;
            segment_ts.push_back(split_t);

            len_accum -= segment_length;
        }

        while (len_accum >= segment_length) {
            split_t += (sampled_t - prev_t) * segment_length / segment_len;
            len_accum -= segment_length;
            segment_ts.push_back(split_t);
        }

        prev_t = sampled_t;
        prev_point = curr_point;
    }

    // drop the last segment if it would otherwise result in a very short piece
    if (len_accum < min_length) {
        segment_ts.pop_back();
    }

    curve.segmentize(segment_ts.begin(), segment_ts.end(),
                     std::back_inserter(segments));
}


}
