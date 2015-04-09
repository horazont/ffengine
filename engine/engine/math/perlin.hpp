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
