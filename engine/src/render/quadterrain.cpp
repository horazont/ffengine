#include "engine/render/quadterrain.hpp"

namespace engine {

inline void push_quad(unsigned int &start_index,
                      std::vector<unsigned int> &indicies)
{
    const unsigned int index_base = start_index;
    start_index += 4;

    indicies.push_back(index_base+0);
    indicies.push_back(index_base+2);
    indicies.push_back(index_base+1);

    indicies.push_back(index_base+1);
    indicies.push_back(index_base+2);
    indicies.push_back(index_base+3);
}

void create_geometry(
        sim::QuadNode *root,
        sim::QuadNode *subtree_root,
        unsigned int lod,
        const std::array<unsigned int, 4> &neighbour_lod,
        std::vector<unsigned int> &indicies,
        std::vector<Vector3f> &positions,
        std::vector<Vector3f> &normals,
        std::vector<Vector3f> &tangents)
{
    switch (subtree_root->type())
    {
    case sim::QuadNode::Type::LEAF:
    {
        unsigned int index_base = positions.size();
        const float size = subtree_root->size();
        const sim::terrain_height_t height = subtree_root->height();
        const Vector3f base(subtree_root->x0(),
                            subtree_root->y0(),
                            subtree_root->height());

        Vector3f nw, ne, sw, se;

        sim::QuadNode *neigh_west = subtree_root->neighbour(sim::QuadNode::WEST);
        sim::QuadNode *neigh_north = subtree_root->neighbour(sim::QuadNode::NORTH);
        sim::QuadNode *neigh_south = subtree_root->neighbour(sim::QuadNode::SOUTH);
        sim::QuadNode *neigh_east = subtree_root->neighbour(sim::QuadNode::EAST);

        if (neigh_west && neigh_west->size() == size) {
            // for west, this is sufficient; west would be responsible for
            // making faces with us if we are at the same subdivision level
            neigh_west = nullptr;
        }
        if (neigh_north && neigh_north->size() == size) {
            // for north, this is sufficient; north would be responsible for
            // making faces with us if we are at the same subdivision level
            neigh_north = nullptr;
        }

        if (neigh_south
                && neigh_south->size() == size
                && neigh_south->type() != sim::QuadNode::Type::LEAF)
        {
            // south neighbour has higher resolution than we have, removing
            // it.
            neigh_south = nullptr;
        }

        if (neigh_east
                && neigh_east->size() == size
                && neigh_east->type() != sim::QuadNode::Type::LEAF)
        {
            // east neighbour has higher resolution than we have, removing
            // it.
            neigh_east = nullptr;
        }

        nw = base;
        ne = base + Vector3f(size-1, 0, 0);
        sw = base + Vector3f(0, size-1, 0);
        se = sw + Vector3f(size-1, 0, 0);

        if (neigh_south && neigh_south->height() == height)
        {
            // extend geometry if neighbour has same height
            sw[eY] += 1;
            se[eY] += 1;
            neigh_south = nullptr;
        }

        if (neigh_east && neigh_east->height() == height)
        {
            // extend geometry if neighbour has same height
            se[eX] += 1;
            ne[eX] += 1;
            neigh_east = nullptr;
        }

        positions.emplace_back(nw);
        positions.emplace_back(ne);
        positions.emplace_back(sw);
        positions.emplace_back(se);

        push_quad(index_base, indicies);

        if (neigh_east) {
            const Vector3f &face_nw = ne;
            const Vector3f &face_sw = se;
            const Vector3f face_ne(face_nw[eX]+1, face_nw[eY], neigh_east->height());
            const Vector3f face_se(face_sw[eX]+1, face_sw[eY], face_ne[eZ]);

            positions.emplace_back(face_nw);
            positions.emplace_back(face_ne);
            positions.emplace_back(face_sw);
            positions.emplace_back(face_se);

            push_quad(index_base, indicies);
        }

        if (neigh_south) {
            const Vector3f &face_nw = sw;
            const Vector3f &face_ne = se;
            const Vector3f face_sw(face_nw[eX], face_nw[eY]+1, neigh_south->height());
            const Vector3f face_se(face_ne[eX], face_sw[eY], face_sw[eZ]);

            positions.emplace_back(face_nw);
            positions.emplace_back(face_ne);
            positions.emplace_back(face_sw);
            positions.emplace_back(face_se);

            push_quad(index_base, indicies);
        }

        if (neigh_south && neigh_east) {
            sim::QuadNode *neigh_southeast = subtree_root->neighbour(sim::QuadNode::SOUTHEAST);
            const Vector3f &face_nw = se;
            const Vector3f face_ne(face_nw[eX]+1, face_nw[eY], neigh_east->height());
            const Vector3f face_sw(face_nw[eX], face_nw[eY]+1, neigh_south->height());
            const Vector3f face_se(face_ne[eX], face_sw[eY], neigh_southeast->height());

            positions.emplace_back(face_nw);
            positions.emplace_back(face_ne);
            positions.emplace_back(face_sw);
            positions.emplace_back(face_se);

            push_quad(index_base, indicies);
        }

        break;
    }
    case sim::QuadNode::Type::NORMAL:
    {
        for (unsigned int i = 0; i < 4; i++) {
            create_geometry(root, subtree_root->child(i), lod, neighbour_lod,
                            indicies,
                            positions, normals, tangents);
        }
        break;
    }
    }
}

}
