/**********************************************************************
File name: plane.hpp
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
#ifndef SCC_RENDER_PLANE_H
#define SCC_RENDER_PLANE_H

#include "ffengine/render/scenegraph.hpp"


namespace ffe {

class Material;

class PlaneNode: public scenegraph::Node
{
    // using a normal node here, the plane is infinitely large, no point in using
    // the octree
public:
    PlaneNode(const Plane &plane,
              Material &material,
              const float size = 10000.f);

private:
    Plane m_plane;
    float m_size;
    bool m_plane_changed;

    Material &m_material;

    IBOAllocation m_ibo_alloc;
    VBOAllocation m_vbo_alloc;

public:
    void set_plane(const Plane &plane);

public:
    void prepare(RenderContext &context);
    void render(RenderContext &context);
    void sync();

};

}

#endif
