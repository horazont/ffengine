/**********************************************************************
File name: network_debug.hpp
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
#ifndef SCC_RENDER_NETWORK_DEBUG_H
#define SCC_RENDER_NETWORK_DEBUG_H

#include "ffengine/sim/network.hpp"

#include "ffengine/render/scenegraph.hpp"


namespace ffe {

class Material;


class DebugNode: public RenderableOctreeObject
{
public:
    DebugNode(const sim::object_ptr<sim::PhysicalNode> &node);

private:
    const sim::object_ptr<sim::PhysicalNode> m_node;

public:
    inline sim::object_ptr<sim::PhysicalNode> node() const
    {
        return m_node;
    }

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;

};


class DebugNodes: public scenegraph::OctNode, public RenderableOctreeObject
{
public:
    DebugNodes(Octree &octree, ffe::Material &material);

private:
    ffe::Material &m_material;
    IBOAllocation m_ibo_alloc;
    VBOAllocation m_vbo_alloc;
    bool m_changed;

    std::unordered_map<sim::object_ptr<sim::PhysicalNode>, std::unique_ptr<DebugNode>> m_nodes;

private:
    void cleanup_dead();

public:
    void register_node(const sim::object_ptr<sim::PhysicalNode> &node);

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync(scenegraph::OctContext &positioning) override;
};


class DebugEdgeBundle: public scenegraph::OctNode, public RenderableOctreeObject
{
public:
    DebugEdgeBundle(Octree &octree,
                    ffe::Material &material,
                    const sim::PhysicalEdgeBundle &bundle);

private:
    ffe::Material &m_material;
    IBOAllocation m_ibo_alloc;
    VBOAllocation m_vbo_alloc;

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync(scenegraph::OctContext &positioning) override;

};


}

#endif
