#include "engine/math/perlin.hpp"

#include "engine/math/algo.hpp"

/* PerlinNoiseGenerator */

PerlinNoiseGenerator::PerlinNoiseGenerator(
        const Vector3 &offset,
        const Vector3 &scale,
        const VectorFloat persistence,
        const unsigned int octaves,
        const VectorFloat largest_feature):
    m_offset(offset),
    m_scale(scale),
    m_persistence(persistence),
    m_octaves(octaves),
    m_base_frequency(1./largest_feature)
{

}

VectorFloat PerlinNoiseGenerator::get(const Vector2 &pos) const
{
    VectorFloat result = m_offset[eZ];
    VectorFloat frequency = m_base_frequency;
    VectorFloat amplitude = m_scale[eZ];
    Vector2 position(pos);
    position[eX] *= m_scale[eX];
    position[eY] *= m_scale[eY];
    position[eX] += m_offset[eX];
    position[eY] += m_offset[eY];

    for (unsigned int level = 0; level < m_octaves; level++) {
        const Vector2 oct_pos(position * frequency);
        result += perlin_rng(oct_pos[eX], oct_pos[eY]) * amplitude;
        frequency *= 2.0;
        amplitude *= m_persistence;
    }

    return result;
}

VectorFloat perlin_rng(const int x, const int y)
{
    const int n = x + y * 57;
    const int m = (n << 13) ^ n;
    return (1.0 - double((m * (m * m * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

VectorFloat perlin_rng_interpolated(const Vector2 &pos)
{
    const int int_x = floor(pos[eX]);
    const int int_y = floor(pos[eY]);

    const VectorFloat frac_x = abs(pos[eX] - VectorFloat(int_x));
    const VectorFloat frac_y = abs(pos[eY] - VectorFloat(int_y));

    const double vs[4] = {
        perlin_rng(int_x   ,   int_y    ),
        perlin_rng(int_x+1 ,   int_y    ),
        perlin_rng(int_x   ,   int_y+1  ),
        perlin_rng(int_x+1 ,   int_y+1  )
    };

    const double ivs[2] = {
        interp_linear(vs[1], vs[0], frac_x),
        interp_linear(vs[3], vs[2], frac_x)
    };

    return interp_linear(ivs[1], ivs[0], frac_y);
}
