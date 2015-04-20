/**********************************************************************
File name: algo.hpp
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
#ifndef SCC_ENGINE_MATH_ALGO_H
#define SCC_ENGINE_MATH_ALGO_H

#include <cmath>
#include <tuple>


template <typename T>
static inline T sqr(T v)
{
    return v*v;
}

template <typename float_t>
static inline float_t frac(float_t v)
{
    return (v - std::trunc(v));
}

/**
 * Return \a v0 if \a t is less than 0.5, otherwise return \a v1.
 */
template <typename T>
static inline T interp_nearest(T v0, T v1, T t)
{
    return (t >= 0.5 ? v1 : v0);
}

/**
 * Interpolate linearily from \a v0 (``t=0``) to \a v1 (``t=1``).
 */
template <typename T>
static inline T interp_linear(T v0, T v1, T t)
{
    return (1.0-t)*v0 + t*v1;
}

/**
 * Interpolate smoothly with cosine from \a v0 (``t=0``) to \a v1 (``t=1``).
 */
template <typename T>
static inline T interp_cos(T v0, T v1, T t)
{
    const T cos_factor = T(1) - sqr(std::cos(t*M_PI_2));
    return interp_linear(v0, v1, cos_factor);
}

/**
 * Clamp \a v to the range from \a low to \a high.
 */
template <typename T>
static inline T clamp(T v, T low, T high)
{
    return std::max(std::min(v, high), low);
}

/**
 * Return -1 if \a v is less than 0, 1 if \a v is greater than 0 and 0
 * otherwise.
 */
template <typename numeric_t>
static inline numeric_t sgn(numeric_t v)
{
    if (v < numeric_t(0)) {
        return numeric_t(-1);
    } else if (v > numeric_t(0)) {
        return numeric_t(1);
    } else {
        return numeric_t(0);
    }
}


/**
 * Rasterize a line using subpixel DDA, iterator style.
 *
 * Example usage:
 *
 *     for (auto iter = RasterIterator<float>(0, 0, 10, 10);
 *          iter;
 *          ++iter)
 *     {
 *         int x, y;
 *         std::tie(x, y) = *iter;
 *         std::cout << x << " " << y << std::endl;
 *     }
 */
template <typename float_t, typename int_t = int>
struct RasterIterator
{
public:
    typedef std::tuple<float_t, float_t> value_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef void difference_type;
    typedef value_type* pointer;
    typedef value_type& reference;

public:
    /**
     * Create an invalid iterator.
     */
    RasterIterator():
        m_step_x(0),
        m_step_y(0),
        m_dxdt(0),
        m_dydt(0),
        m_x(NAN),
        m_y(NAN),
        m_t_nextx(NAN),
        m_t_nexty(NAN)
    {

    }

    /**
     * Create a raster iterator from the point ``(x0, y0)`` to ``(x1, y1)``.
     *
     * The iterator iterates over all integer squares which are touched by the
     * line from the starting to the end point.
     */
    RasterIterator(float x0, float y0,
                   float x1, float y1):
        m_step_x(x1 > x0 ? 1 : x1 == x0 ? 0 : -1),
        m_step_y(y1 > y0 ? 1 : y1 == y0 ? 0 : -1),
        m_dxdt(m_step_x / (x1-x0)),
        m_dydt(m_step_y / (y1-y0)),
        m_x(x0),
        m_y(y0),
        m_t_nextx(),
        m_t_nexty()
    {
        if (isnan(m_dxdt)) {
            m_t_nextx = 1;
        } else if (x1 > x0) {
            m_t_nextx = m_dxdt * (1.0 - frac(x0));
        } else {
            m_t_nextx = m_dxdt * (frac(x0));
        }
        if (isnan(m_dydt)) {
            m_t_nexty = 1;
        } else if (y1 > y0) {
            m_t_nexty = m_dydt * (1.0 - frac(y0));
        } else {
            m_t_nexty = m_dydt * (frac(y0));
        }
    }

    RasterIterator(const RasterIterator &ref) = default;
    RasterIterator &operator=(const RasterIterator &ref) = default;

private:
    float m_step_x, m_step_y;
    float m_dxdt, m_dydt;
    float m_x, m_y;
    float m_t_nextx, m_t_nexty;

public:
    /**
     * Advance the iterator
     */
    RasterIterator &operator++()
    {
        if (!(*this)) {
            return *this;
        }

        if (m_t_nextx < 1 || m_t_nexty < 1)
        {
            if (m_t_nextx < m_t_nexty) {
                m_t_nextx += m_dxdt;
                m_x += m_step_x;
            } else {
                m_t_nexty += m_dydt;
                m_y += m_step_y;
            }
        } else {
            m_x = NAN;
            m_y = NAN;
            m_t_nextx = NAN;
            m_t_nexty = NAN;
        }

        return *this;
    }

    /**
     * Return an advanced copy of the iterator
     */
    RasterIterator operator++(int) const
    {
        RasterIterator result(*this);
        ++result;
        return result;
    }

    /**
     * Return true if the iterator is valid, false otherwise.
     *
     * An iterator which is not valid compares equal to all other invalid
     * iterators.
     */
    operator bool() const
    {
        return !isnan(m_x);
    }

    /**
     * Compare iterators. Iterators only compare equal if they are both
     * invalid.
     */
    bool operator==(const RasterIterator &other) const
    {
        if (!(*this) && !other) {
            return true;
        }
        return false;
    }

    /**
     * Return the square at which the iterator currently points.
     */
    std::tuple<int_t, int_t> operator*() const
    {
        return std::make_tuple(std::trunc(m_x),
                               std::trunc(m_y));
    }

};

#endif
