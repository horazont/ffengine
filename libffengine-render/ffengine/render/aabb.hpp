/**********************************************************************
File name: aabb.hpp
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
#ifndef SCC_ENGINE_RENDER_AABB_H
#define SCC_ENGINE_RENDER_AABB_H

#include <functional>
#include <vector>

#include "ffengine/math/aabb.hpp"

#include "ffengine/render/scenegraph.hpp"
#include "ffengine/render/renderpass.hpp"


namespace ffe {

class DynamicAABBs: public scenegraph::Node
{
public:
    using DiscoverCallback = std::function<void(std::vector<AABB>&)>;

public:
    DynamicAABBs(Material &mat, DiscoverCallback &&cb);

private:
    Material &m_material;

    std::vector<AABB> m_aabbs;
    DiscoverCallback m_discover_cb;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync() override;

};

}

#endif
