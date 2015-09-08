/**********************************************************************
File name: curve.hpp
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
#ifndef SCC_ENGINE_RENDER_CURVE_H
#define SCC_ENGINE_RENDER_CURVE_H

#include "ffengine/math/curve.hpp"

#include "ffengine/render/renderpass.hpp"


namespace ffe {

class QuadBezier3fDebug: public scenegraph::OctNode,
                         public RenderableOctreeObject
{
public:
    QuadBezier3fDebug(Material &mat, unsigned int steps);

private:
    Material &m_mat;
    unsigned int m_steps;
    QuadBezier3f m_curve;
    bool m_curve_changed;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    inline const QuadBezier3f &curve() const
    {
        return m_curve;
    }

    inline void set_curve(const QuadBezier3f &curve)
    {
        m_curve = curve;
        m_curve_changed = true;
    }

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync(ffe::Octree &octree,
              scenegraph::OctContext &positioning) override;

};

class QuadBezier3fRoadTest: public scenegraph::OctNode,
                         public RenderableOctreeObject
{
public:
    QuadBezier3fRoadTest(Material &mat, unsigned int steps);

private:
    Material &m_mat;
    unsigned int m_steps;
    QuadBezier3f m_curve;
    bool m_curve_changed;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    inline const QuadBezier3f &curve() const
    {
        return m_curve;
    }

    inline void set_curve(const QuadBezier3f &curve)
    {
        m_curve = curve;
        m_curve_changed = true;
    }

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync(ffe::Octree &octree,
              scenegraph::OctContext &positioning) override;

};

}

#endif
