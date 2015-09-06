/**********************************************************************
File name: pointer.hpp
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
#ifndef SCC_ENGINE_RENDER_POINTER_H
#define SCC_ENGINE_RENDER_POINTER_H

#include "ffengine/render/scenegraph.hpp"

namespace ffe {

/**
 * Draw a transparent cube with given radius.
 */
class PointerNode: public scenegraph::Node
{
public:
    /**
     * @opengl
     */
    PointerNode(const float radius);

private:
    ffe::Material m_material;

    ffe::VBOAllocation m_vbo_alloc;
    ffe::IBOAllocation m_ibo_alloc;

public:
    void render(RenderContext &context) override;
    void sync(RenderContext &context) override;

};

}

#endif // SCC_ENGINE_RENDER_POINTER_H

