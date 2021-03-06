/**********************************************************************
File name: grid.hpp
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
#ifndef SCC_ENGINE_RENDER_GRID_H
#define SCC_ENGINE_RENDER_GRID_H

#include "ffengine/render/scenegraph.hpp"
#include "ffengine/render/renderpass.hpp"


namespace ffe {

/**
 * Render a line grid, coloured based on coordinates.
 */
class GridNode: public scenegraph::Node
{
public:
    /**
     * Create a grid node.
     *
     * @param xcells Number of cells in the X direction
     * @param ycells Number of cells in the Y direction
     * @param size Size of each cell
     *
     * @opengl
     */
    GridNode(Material &mat,
             const unsigned int xcells,
             const unsigned int ycells,
             const float size);

private:
    Material &m_material;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    void render(RenderContext &context) override;
    void prepare(RenderContext &context) override;

};

}

#endif // SCC_ENGINE_RENDER_GRID_H

