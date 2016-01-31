/**********************************************************************
File name: mixedcurve.hpp
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
#ifndef SCC_ENGINE_MATH_MIXEDCURVE_H
#define SCC_ENGINE_MATH_MIXEDCURVE_H

#include "ffengine/math/curve.hpp"


template <typename float_t>
class MixedCurve
{
public:
    using vector3_t = Vector<float_t, 3>;
    using vector2_t = Vector<float_t, 2>;
    using curve_t = CubeBezier<vector3_t>;

public:
    MixedCurve() = default;

    template <typename other_float_t>
    explicit MixedCurve(const QuadBezier<Vector<other_float_t, 3> > &src):
        m_curve(src.p_start,
                src.p_start + 2.f/3.f*(src.p_control - src.p_start),
                src.p_end + 2.f/3.f*(src.p_control - src.p_end),
                src.p_end),
        m_xy_control(src.p_control)
    {

    }

private:
    curve_t m_curve;
    vector2_t m_xy_control;

    inline void update_controls()
    {
        const vector2_t xy_start(m_curve.p_start);
        const vector2_t xy_end(m_curve.p_end);
        m_curve.p_control1 = vector3_t(xy_start + 2.f/3.f*(m_xy_control - xy_start), m_curve.p_control1[eZ]);
        m_curve.p_control2 = vector3_t(xy_end + 2.f/3.f*(m_xy_control - xy_end), m_curve.p_control2[eZ]);
    }

public:
    template <typename other_float_t>
    inline void set_start(const Vector<other_float_t, 3> &start)
    {
        m_curve.p_start = start;
        update_controls();
    }

    template <typename other_float_t>
    inline void set_end(const Vector<other_float_t, 3> &end)
    {
        m_curve.p_end = end;
        update_controls();
    }

    template <typename float_t1, typename float_t2, typename float_t3>
    inline void set_control(const Vector<float_t1, 2> &xy,
                            const float_t2 z1,
                            const float_t3 z2)
    {
        m_xy_control = xy;
        m_curve.p_control1[eZ] = z1;
        m_curve.p_control2[eZ] = z2;
        update_controls();
    }

    template <typename other_float_t>
    inline void set_control(const Vector<other_float_t, 3> &qcontrol)
    {
        const float_t z1 = m_curve.p_start[eZ] + 2.f/3.f*(qcontrol[eZ] - m_curve.p_start[eZ]);
        const float_t z2 = m_curve.p_end[eZ] + 2.f/3.f*(qcontrol[eZ] - m_curve.p_end[eZ]);
        set_control(Vector<other_float_t, 2>(qcontrol), z1, z2);
    }

    template <typename other_float_t>
    inline void set_qcurve(const QuadBezier<Vector<other_float_t, 3> > &curve)
    {
        m_curve.p_start = curve.p_start;
        m_curve.p_end = curve.p_end;
        m_xy_control = vector2_t(curve.p_control);
        m_curve.p_control1[eZ] = m_curve.p_control2[eZ] = curve.p_control[eZ];
        update_controls();
    }

public:
    const curve_t &curve() const
    {
        return m_curve;
    }

    QuadBezier<vector2_t> curve_2d() const
    {
        return QuadBezier<vector2_t>(vector2_t(m_curve.p_start),
                                     m_xy_control,
                                     vector2_t(m_curve.p_end));
    }

};


using MixedCurvef = MixedCurve<float>;
using MixedCurved = MixedCurve<double>;


#endif
