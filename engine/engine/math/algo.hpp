#ifndef SCC_ENGINE_MATH_ALGO_H
#define SCC_ENGINE_MATH_ALGO_H

#include <cmath>

template <typename T>
static inline T sqr(T v)
{
    return v*v;
}

template <typename T>
static inline T interp_nearest(T v0, T v1, T t)
{
    return (t >= 0.5 ? v1 : v0);
}

template <typename T>
static inline T interp_linear(T v0, T v1, T t)
{
    return t*v0 + (1.0-t)*v1;
}

template <typename T>
static inline T interp_cos(T v0, T v1, T t)
{
    const double cos_factor = sqr(std::cos(t*M_PI_2));
    return interp_linear(v0, v1, cos_factor);
}

#endif
