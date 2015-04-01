#include "engine/sim/terrain.hpp"

namespace sim {

Terrain::Terrain(const unsigned int width,
                 const unsigned int height):
    m_width(width),
    m_height(height),
    m_heightmap(m_width*m_height, default_height)
{

}

}
