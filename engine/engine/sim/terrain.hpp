#ifndef SCC_SIM_TERRAIN_H
#define SCC_SIM_TERRAIN_H

#include <cstdint>
#include <string>

namespace sim {

struct Terrain
{
public:
    typedef std::uint8_t height_t;
    static const height_t default_height = 20;
    static const height_t watermark = 25;

public:
    Terrain(const unsigned int width,
            const unsigned int height);

public:
    const unsigned int m_width;
    const unsigned int m_height;
    float m_offset;
    std::basic_string<height_t> m_heightmap;

public:
    inline height_t get(unsigned int x, unsigned int y) const
    {
        return m_heightmap[y*m_height+x];
    }

};

}

#endif
