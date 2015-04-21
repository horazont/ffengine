/**********************************************************************
File name: world.hpp
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
#ifndef SCC_SIM_WORLD_H
#define SCC_SIM_WORLD_H

#include "engine/sim/terrain.hpp"


namespace sim {


/**
 * Accessor interface to access the state related to the terrain. As always,
 * accessor interfaces only provide read-only access.
 */
class IWorldTerrainAccessor
{
public:
    /**
     * Return a const reference to the terrain.
     */
    virtual const Terrain &terrain() const = 0;

};


/**
 * Mutator interface to access the state related to terraforming.
 *
 * This includes basic terraforming activity.
 */
class IWorldTerraformingMutator
{
public:
    /**
     * Raise the terrain around \a xc, \a yc.
     *
     * This uses the given brush, determined by the \a brush_size and the
     * \a density_map, multiplied with \a brush_strength. \a brush_strength
     * may be negative to create a lowering effect.
     *
     * @param xc X center of the effect
     * @param yc Y center of the effect
     * @param brush_size Diameter of the brush
     * @param density_map Density values of the brush (must have \a brush_size
     * times \a brush_size entries).
     * @param brush_strength Strength factor for applying the brush, should be
     * in the range [-1.0, 1.0].
     */
    virtual void tf_raise(const float xc, const float yc,
                          const unsigned int brush_size,
                          const std::vector<float> &density_map,
                          const float brush_strength) = 0;

    /**
     * Level the terrain around \a xc, \a yc to a specific reference height
     * \a ref_height.
     *
     * This uses the given brush, determined by the \a brush_size and the
     * \a density_map, multiplied with \a brush_strength. Using a negative
     * brush strength will have the same effect as using a negative
     * \a ref_height and will lead to clipping on the lower end of the terrain
     * height dynamic range.
     *
     * @param xc X center of the effect
     * @param yc Y center of the effect
     * @param brush_size Diameter of the brush
     * @param density_map Density values of the brush (must have \a brush_size
     * times \a brush_size entries).
     * @param brush_strength Strength factor for applying the brush, should be
     * in the range [0.0, 1.0].
     * @param ref_height Reference height to level the terrain to.
     */
    virtual void tf_level(const float xc, const float yc,
                          const unsigned int brush_size,
                          const std::vector<float> &density_map,
                          const float brush_strength,
                          const float ref_height) = 0;

};


class IWorldTerraformAccessor: public IWorldTerrainAccessor
{
};

class IWorldTerraformMutator: public IWorldTerraformingMutator
{
};


class TerraformWorld: public IWorldTerraformMutator, public IWorldTerraformAccessor
{
public:
    TerraformWorld();

private:
    Terrain m_terrain;

protected:
    void notify_update_terrain_rect(const float xc,
                                    const float yc,
                                    const unsigned int size);

public:
    const Terrain &terrain() const override;

    inline Terrain &mutable_terrain()
    {
        return m_terrain;
    }

    void tf_raise(const float xc, const float yc,
                  const unsigned int brush_size,
                  const std::vector<float> &density_map,
                  const float brush_strength) override;
    void tf_level(const float xc, const float yc,
                  const unsigned int brush_size,
                  const std::vector<float> &density_map,
                  const float brush_strength,
                  const float ref_height) override;
};

}

#endif
