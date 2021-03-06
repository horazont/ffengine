/**********************************************************************
File name: curve.hpp
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
#ifndef SCC_ENGINE_MATH_CURVE_H
#define SCC_ENGINE_MATH_CURVE_H

#include <functional>
#include <ostream>

#include "ffengine/math/algo.hpp"
#include "ffengine/math/vector.hpp"


template <typename curve_t, typename InputIterator, typename OutputIterator>
inline void segmentize(const curve_t &curve,
                       InputIterator iter, InputIterator end,
                       OutputIterator dest)
{
    curve_t remaining_curve(curve);

    float_t t_offset = 0.;
    float_t t_scale = 1.;
    for (; iter != end; ++iter) {
        const float_t segment_t = *iter;
        const float_t split_t = (segment_t + t_offset) * t_scale;
        t_offset = -segment_t;
        t_scale = 1./(1-segment_t);

        std::tie(*dest++, remaining_curve) = remaining_curve.split(split_t);
    }

    *dest++ = remaining_curve;
}


template <typename _vector_t>
struct QuadBezier
{
    typedef _vector_t vector_t;
    typedef typename vector_t::vector_float_t float_t;

    QuadBezier():
        p_start(),
        p_control(),
        p_end()
    {

    }

    template <typename p1_t, typename p2_t, typename p3_t>
    QuadBezier(p1_t &&p1, p2_t &&p2, p3_t &&p3):
        p_start(p1),
        p_control(p2),
        p_end(p3)
    {

    }

    vector_t p_start, p_control, p_end;

    // printing

    template <typename other_vector_t>
    inline bool operator==(const QuadBezier<other_vector_t> &other) const
    {
        return (p_start == other.p_start) && (p_control == other.p_control) && (p_end == other.p_end);
    }

    template <typename other_vector_t>
    inline bool operator!=(const QuadBezier<other_vector_t> &other) const
    {
        return !(*this == other);
    }

    // splitting

    inline QuadBezier split_inplace(const float_t t)
    {
        const vector_t p2_1 = (*this)[t];
        const vector_t p2_2 = p_control + t*(p_end - p_control);
        const vector_t p2_3 = p_end;

        p_control = p_start + t*(p_control - p_start);
        p_end = p2_1;

        return QuadBezier(p2_1, p2_2, p2_3);
    }

    inline std::pair<QuadBezier, QuadBezier> split(const float_t t) const
    {
        QuadBezier c1(*this);
        const QuadBezier c2 = c1.split_inplace(t);
        return std::make_pair(c1, c2);
    }

    template <typename InputIterator, typename OutputIterator>
    inline void segmentize(InputIterator iter, InputIterator end,
                           OutputIterator dest) const
    {
        ::segmentize(*this, iter, end, dest);
    }

    // evaluating

    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> operator[](float_t t) const
    {
        return sqr(1-t)*p_start + 2*(1-t)*t*p_control + sqr(t)*p_end;
    }


    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> diff(float_t t) const
    {
        return 2*(1-t)*(p_control - p_start) + 2*t*(p_end - p_control);
    }

};


template<typename _vector_t>
struct CubeBezier
{
    typedef _vector_t vector_t;
    typedef typename vector_t::vector_float_t float_t;

    CubeBezier()
    {

    }

    template <typename p_1, typename p_2, typename p_3, typename p_4>
    CubeBezier(p_1 &&start, p_2 &&control1, p_3 &&control2, p_4 &&end):
        p_start(std::forward<p_1>(start)),
        p_control1(std::forward<p_2>(control1)),
        p_control2(std::forward<p_3>(control2)),
        p_end(std::forward<p_4>(end))
    {

    }

    vector_t p_start, p_control1, p_control2, p_end;

    // equality

    template <typename other_vector_t>
    bool operator==(const CubeBezier<other_vector_t> &other) const
    {
        return (p_start == other.p_start &&
                p_control1 == other.p_control1 &&
                p_control2 == other.p_control2 &&
                p_end == other.p_end);
    }

    template <typename other_vector_t>
    bool operator!=(const CubeBezier<other_vector_t> &other) const
    {
        return !(*this == other);
    }

    // splitting

    inline CubeBezier split_inplace(const float_t t)
    {
        const vector_t &c1 = p_start;
        const vector_t c2 = p_start + (p_control1-p_start)*t;
        const vector_t c4 = p_control1 + (p_control2-p_control1)*t;
        const vector_t c3 = c2 + (c4-c2)*t;
        const vector_t c6 = p_control2 + (p_end-p_control2)*t;
        const vector_t c5 = c4 + (c6-c4)*t;
        const vector_t &c7 = p_end;

        const vector_t p2_start = c3 + (c5-c3)*t;
        const vector_t &p2_control1 = c5;
        const vector_t &p2_control2 = c6;
        const vector_t p2_end = p_end;

        p_control1 = c2;
        p_control2 = c3;
        p_end = p2_start;

        return CubeBezier(p2_start, p2_control1, p2_control2, p2_end);
    }

    std::tuple<CubeBezier, CubeBezier> split(const float_t t) const
    {
        CubeBezier master = *this;
        const CubeBezier splitoff = master.split_inplace(t);
        return std::make_tuple(master, splitoff);
    }

    template <typename InputIterator, typename OutputIterator>
    inline void segmentize(InputIterator iter, InputIterator end,
                           OutputIterator dest) const
    {
        ::segmentize(*this, iter, end, dest);
    }

    // evaluating

    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> operator[](const float_t t) const
    {
        const float_t t_inv = 1 - t;
        const float_t t_inv_p2 = t_inv * t_inv;
        const float_t t_inv_p3 = t_inv_p2 * t_inv;

        const float_t t_p2 = t*t;
        const float_t t_p3 = t_p2*t;

        return p_start*t_inv_p3 + p_control1*3*t_inv_p2*t + p_control2*3*t_inv*t_p2 + p_end*t_p3;
    }

    template <typename float_t>
    inline Vector<float_t, vector_t::dimension> diff(const float_t t) const
    {
        const float_t t_inv = t - 1;
        const float_t t_inv_p2 = t_inv * t_inv;

        const float_t t_p2 = t*t;

        return 3*(p_control1-p_start)*t_inv_p2 + 6*(p_control1-p_control2)*t*t_inv + 3*(p_end-p_control2)*t_p2;
    }
};


typedef QuadBezier<Vector3f> QuadBezier3f;
typedef QuadBezier<Vector3d> QuadBezier3d;

typedef CubeBezier<Vector3f> CubeBezier3f;
typedef CubeBezier<Vector3d> CubeBezier3d;


template <typename curve_t>
inline float bisect_curve(const curve_t &curve,
                          std::function<int(const curve_t&, float)> predicate,
                          float t_min,
                          float t_max)
{
    while (t_max - t_min >= 1e-6) {
        const float t_center = (t_max + t_min) / 2;
        /* std::cout << "t_center = " << t_center << std::endl; */
        const int predicate_result = predicate(curve, t_center);
        if (predicate_result == 0) {
            return t_center;
        } else if (predicate_result > 0) {
            t_max = t_center;
        } else {
            t_min = t_center;
        }
    }
    return t_max;
}

template <typename curve_t>
inline float bisect_curve_length(const curve_t &curve,
                                 const typename curve_t::vector_t &origin,
                                 const float t_min,
                                 const float t_max,
                                 const float distance,
                                 const float epsilon)
{
    std::function<int(const curve_t &curve, float t)> predicate = [distance, epsilon, &origin](
            const curve_t &curve,
            float t) {
        const typename curve_t::vector_t pos = curve[t];
        const float curr_distance = (pos - origin).length();
        if (std::abs(curr_distance - distance) <= epsilon) {
            return 0;
        } else if (curr_distance > distance) {
            return 1;
        } else {
            return -1;
        }
    };
    return bisect_curve(curve, predicate, t_min, t_max);
}


template <typename curve_t>
inline float bisect_curve_tangent_angle(const curve_t &curve,
                                        const typename curve_t::vector_t &tangent,
                                        const float t_min,
                                        const float t_max,
                                        const float angle,
                                        const float epsilon)
{
    std::function<int(const curve_t &curve, float t)> predicate = [angle, epsilon, &tangent](
            const curve_t &curve,
            float t) -> int {
        const typename curve_t::vector_t curr_tangent = curve.diff(t).normalized();
        const float curr_angle = std::acos(curr_tangent * tangent);
        /*  std::cout << "tangent = " << tangent << "; " << "curr_tangent = " << curr_tangent << "; dotp = " << (curr_tangent * tangent) << "; α = " << curr_angle << std::endl; */
        if (std::abs(curr_angle - angle) <= epsilon) {
            return 0;
        } else if (curr_angle > angle) {
            return 1;
        } else {
            return -1;
        }
    };
    return bisect_curve(curve, predicate, t_min, t_max);
}


template <typename curve_t, typename OutputIterator>
inline void autosample_curve(const curve_t &curve,
                             OutputIterator dest,
                             const float threshold = 0.01,
                             const float min_length = 0.001,
                             const float epsilon = 0.0001)
{
    using bezier_vector_t = typename curve_t::vector_t;
    float t = 0;
    bezier_vector_t prev_position = curve[t];
    bezier_vector_t prev_tangent = curve.diff(t).normalized();
    bezier_vector_t next_tangent;
    bezier_vector_t next_position;
    float t_next;
    *dest++ = t;
    for (;; t = t_next, *dest++ = t, prev_tangent = next_tangent, prev_position = next_position) {
        /* std::cout << "iterate! t=" << t << std::endl; */
        const float tangent_step = prev_tangent.length() / 10.f;
        float t_lower = t;
        t_next = t + tangent_step;

        next_position = curve[t_next];
        if ((next_position - prev_position).length() < min_length) {
            t_next = bisect_curve_length(curve, prev_position, t_lower, t_next, min_length, 0.01);
            t_lower = t_next;
        }

        if (t_next >= 1.f) {
            /* std::cout << "at end of curve" << std::endl; */
            *dest++ = 1.f;
            return;
        }

        next_tangent = curve.diff(t_next).normalized();
        /* std::cout << "prev tangent: " << prev_tangent << std::endl; */
        while (std::acos(prev_tangent * next_tangent) < threshold && t_next < 1.f) {
            t_lower = t_next;
            t_next += tangent_step;
            next_tangent = curve.diff(t_next).normalized();
            /*std::cout << "next tangent: " << next_tangent << std::endl;
            std::cout << "angle " << std::acos(prev_tangent * next_tangent) << std::endl;
            std::cout << "t_lower = " << t_lower << "; t_next = " << t_next << std::endl;*/
        };

        if (t_next >= 1.f) {
            /* std::cout << "at end of curve" << std::endl; */
            *dest++ = 1.f;
            return;
        }

        if (t_lower == t_next) {
            /* std::cout << "at max resolution" << std::endl; */
            next_position = curve[t_next];
            continue;
        }

        /* std::cout << "bisecting" << std::endl; */
        t_next = bisect_curve_tangent_angle(curve, prev_tangent,
                                            t_lower,
                                            t_next,
                                            threshold,
                                            epsilon);
        next_tangent = curve.diff(t_next).normalized();
        next_position = curve[t_next];
    }
}


template <typename curve_t, typename InputIterator>
typename curve_t::float_t sampled_curve_length(const curve_t &curve,
                                               InputIterator iter,
                                               InputIterator end)
{
    if (iter == end) {
        return 0.f;
    }

    typename curve_t::float_t len = 0.f;
    typename curve_t::vector_t previous_point = curve[*iter++];
    for (; iter != end; ++iter) {
        typename curve_t::vector_t next_point = curve[*iter];
        len += (next_point - previous_point).length();
        previous_point = next_point;
    }

    return len;
}


namespace std {

template <typename vector_t>
ostream &operator<<(ostream &stream, const QuadBezier<vector_t> &bezier)
{
    return stream << "bezier("
                  << bezier.p_start << ", "
                  << bezier.p_control << ", "
                  << bezier.p_end << ")";
}

template <typename vector_t>
ostream &operator<<(ostream &stream, const CubeBezier<vector_t> &bezier)
{
    return stream << "bezier("
                  << bezier.p_start << ", "
                  << bezier.p_control1 << ", "
                  << bezier.p_control2 << ", "
                  << bezier.p_end << ")";
}

}

#endif
