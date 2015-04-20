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

template <typename T>
static inline T interp_nearest(T v0, T v1, T t)
{
    return (t >= 0.5 ? v1 : v0);
}

template <typename T>
static inline T interp_linear(T v0, T v1, T t)
{
    return (1.0-t)*v0 + t*v1;
}

template <typename T>
static inline T interp_cos(T v0, T v1, T t)
{
    const double cos_factor = sqr(std::cos(t*M_PI_2));
    return interp_linear(v0, v1, cos_factor);
}

template <typename T>
static inline T clamp(T v, T low, T high)
{
    return std::max(std::min(v, high), low);
}

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

template <typename float_t, typename Callable>
static inline void raster_line_inclusive(
        float_t x0, float_t y0,
        float_t x1, float_t y1,
        Callable &&callable)
{
    // t = 0 => (x = x0, y = y0)
    // t = 1 => (x = x1, y = y1)

    /* assert(x0 <= x1);
    assert(y0 <= y1); */

    float x = x0;
    float y = y0;

    const float step_x = (x1 > x0 ? 1 : -1);
    const float step_y = (y1 > y0 ? 1 : -1);

    const float dxdt = step_x/(x1-x0);
    const float dydt = step_y/(y1-y0);

    const float next_x = std::floor(x*step_x + 1)*step_x;
    const float next_y = std::floor(y*step_y + 1)*step_y;

    /* std::cout << x0 << " " << y0 << std::endl;
    std::cout << next_x << " " << next_y << std::endl; */

    float t_nextx = (next_x - x0) * dxdt * step_x;
    float t_nexty = (next_y - y0) * dydt * step_y;

    /* std::cout << t_nextx << " " << t_nexty << std::endl;
    std::cout << dxdt << " " << dydt << std::endl; */

    /* _Exit(1); */

    callable(std::trunc(x), std::trunc(y));
    do
    {
        if (t_nextx < t_nexty) {
            t_nextx += dxdt;
            x += step_x;
        } else {
            t_nexty += dydt;
            y += step_y;
        }
        /* std::cout << t_nextx << " " << t_nexty << std::endl; */
        callable(std::trunc(x), std::trunc(y));
    } while (t_nextx < 1 || t_nexty < 1);
}


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

    RasterIterator operator++(int) const
    {
        RasterIterator result(*this);
        ++result;
        return result;
    }

    operator bool() const
    {
        return !isnan(m_x);
    }

    bool operator==(const RasterIterator &other) const
    {
        if (!(*this) && !other) {
            return true;
        }
        return false;
    }

    std::tuple<int_t, int_t> operator*() const
    {
        return std::make_tuple(std::trunc(m_x),
                               std::trunc(m_y));
    }

};

#endif
