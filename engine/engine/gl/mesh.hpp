/**********************************************************************
File name: mesh.hpp
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
#ifndef SCC_ENGINE_MATH_MESH_H
#define SCC_ENGINE_MATH_MESH_H

#include "engine/math/vector.hpp"

namespace engine {

struct MeshFace
{
    int32_t verticies[3];
};

struct MeshEdge
{
    int32_t faces[2];
    int8_t face_sides[2];
};

struct MeshVertex
{
    std::vector<int32_t> edges;
    std::vector<int8_t> edge_sides;
};

class RenderableMesh
{
public:
    Mesh();

private:
    /* share the same indicies */
    std::vector<Vector3f> m_positions;
    std::vector<MeshVertex> m_verticies;

    std::vector<MeshFace> m_faces;
    std::vector<MeshEdge> m_edges;

public:
    int32_t add_vertex(const Vector3f &pos);
    int32_t make_face(const int32_t v1, const int32_t v2, const int32_t v3);
    int32_t make_face(const int32_t vs[3]);
};


struct MeshVertexRef
{
    Mesh *mesh;
    int32_t vertex;
};

struct MeshEdgeRef
{
    Mesh *mesh;
    int32_t edge;
};

struct MeshFaceRef
{
    Mesh *mesh;
    int32_t face;
};

}

#endif // SCC_ENGINE_MATH_MESH_H

