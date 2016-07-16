#ifndef SCC_ENGINE_MATH_MESH_H
#define SCC_ENGINE_MATH_MESH_H

#include <cassert>
#include <iostream>
#include <list>
#include <memory>

#include "ffengine/common/stable_index_vector.hpp"
#include "ffengine/math/vector.hpp"


struct NoData
{

};


template <typename HalfedgeMesh>
class AroundVertexIterator
{
public:
    using difference_type = void;
    using value_type = const typename HalfedgeMesh::VertexHandle;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;

public:
    AroundVertexIterator():
        m_center(nullptr),
        m_curr_edge(nullptr),
        m_curr(nullptr),
        m_boundary_reversal(false)
    {

    }

    AroundVertexIterator(const AroundVertexIterator &src) = default;
    AroundVertexIterator &operator=(const AroundVertexIterator &src) = default;

protected:
    explicit AroundVertexIterator(typename HalfedgeMesh::VertexHandle center):
        m_center(center),
        m_curr_edge(center->outgoing()),
        m_curr(m_curr_edge ? m_curr_edge->dest() : nullptr),
        m_boundary_reversal(false)
    {

    }

private:
    typename HalfedgeMesh::VertexHandle m_center;
    typename HalfedgeMesh::HalfedgeHandle m_curr_edge;
    typename HalfedgeMesh::VertexHandle m_curr;
    bool m_boundary_reversal;

    void step_backward()
    {
        m_curr_edge = m_curr_edge->prev()->twin();
    }

    void step_forward()
    {
        if (m_boundary_reversal) {
            m_curr_edge = m_curr_edge->next();
            m_boundary_reversal = false;
            return;
        }

        if (m_curr_edge->twin()) {
            m_curr_edge = m_curr_edge->twin()->next();
        } else {
            m_curr_edge = nullptr;
        }
    }

    void skip_forward_boundary()
    {
        m_curr_edge = m_center->outgoing();
        auto prev = m_curr_edge;
        while (m_curr_edge) {
            prev = m_curr_edge;
            step_backward();
        }
        m_curr_edge = prev->prev();
        m_boundary_reversal = true;
    }

public:
    reference operator*()
    {
        return m_curr;
    }

    value_type *operator->()
    {
        return &m_curr;
    }

    AroundVertexIterator &operator++()
    {
        if (!m_curr_edge) {
            return *this;
        }

        step_forward();
        if (m_curr_edge == nullptr) {
            // we have reached a boundary!
            skip_forward_boundary();
        }

        if (m_curr_edge == m_center->outgoing()) {
            // end
            m_curr_edge = nullptr;
        }
        if (m_curr_edge) {
            if (m_boundary_reversal) {
                m_curr = m_curr_edge->origin();
            } else {
                m_curr = m_curr_edge->dest();
            }
        } else {
            m_curr = nullptr;
        }

        return *this;
    }

    AroundVertexIterator operator++(int)
    {
        AroundVertexIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    explicit operator bool() const
    {
        return bool(m_center) && bool(m_curr_edge);
    }

    bool operator==(const AroundVertexIterator &iter) const
    {
        if (!m_curr_edge and !iter.m_curr_edge) {
            return true;
        }
        return m_center == iter.m_center and m_curr_edge == iter.m_curr_edge;
    }

    bool operator!=(const AroundVertexIterator &iter) const
    {
        return !(*this == iter);
    }

public:
    friend HalfedgeMesh;

};


template <typename HalfedgeMesh>
class FaceVertexIterator
{
public:
    using difference_type = void;
    using value_type = const typename HalfedgeMesh::VertexHandle;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::input_iterator_tag;

public:
    FaceVertexIterator():
        m_first(nullptr),
        m_curr_edge(nullptr),
        m_curr(nullptr)
    {

    }

    FaceVertexIterator(const FaceVertexIterator &src) = default;
    FaceVertexIterator &operator=(const FaceVertexIterator &src) = default;

protected:
    explicit FaceVertexIterator(typename HalfedgeMesh::FaceHandle face):
        m_first((face ? face->first() : nullptr)),
        m_curr_edge(m_first),
        m_curr(m_first ? m_first->dest() : nullptr)
    {

    }

private:
    typename HalfedgeMesh::HalfedgeHandle m_first;
    typename HalfedgeMesh::HalfedgeHandle m_curr_edge;
    typename HalfedgeMesh::VertexHandle m_curr;

public:
    value_type operator*()
    {
        return m_curr;
    }

    pointer operator->()
    {
        return &m_curr;
    }

    FaceVertexIterator &operator++()
    {
        if (!m_curr_edge) {
            return *this;
        }

        m_curr_edge = m_curr_edge->next();
        assert(m_curr_edge);
        if (m_curr_edge == m_first) {
            m_curr_edge = nullptr;
            m_curr = nullptr;
        } else {
            m_curr = m_curr_edge->dest();
        }

        return *this;
    }

    FaceVertexIterator operator++(int)
    {
        FaceVertexIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    explicit operator bool() const
    {
        return bool(m_curr_edge);
    }

    bool operator==(const FaceVertexIterator &iter) const
    {
        if (!m_curr_edge and !iter.m_curr_edge) {
            return true;
        }
        return m_curr_edge == iter.m_curr_edge;
    }

    bool operator!=(const FaceVertexIterator &iter) const
    {
        return !(*this == iter);
    }

public:
    friend HalfedgeMesh;

};


template <typename Iterator>
class MeshElementHandle
{
public:
    MeshElementHandle():
        m_valid(false)
    {

    }

    MeshElementHandle(const Iterator &iter):
        m_valid(true),
        m_iter(iter)
    {

    }

    MeshElementHandle(std::nullptr_t):
        m_valid(false)
    {

    }

    MeshElementHandle(const MeshElementHandle &ref) = default;
    MeshElementHandle &operator=(const MeshElementHandle &ref) = default;

    using value_type = typename Iterator::value_type;
    using reference = typename Iterator::reference;
    using pointer = typename Iterator::pointer;

private:
    bool m_valid;
    Iterator m_iter;

public:
    pointer operator->() const
    {
        return &*m_iter;
    }

    reference operator*() const
    {
        return *m_iter;
    }

    explicit operator bool() const
    {
        return m_valid;
    }

    MeshElementHandle &operator++()
    {
        if (!m_valid) {
            return *this;
        }
        ++m_iter;
        return *this;
    }

    MeshElementHandle operator++(int)
    {
        MeshElementHandle result = *this;
        if (m_valid) {
            ++m_iter;
        }
        return result;
    }

    bool operator==(const MeshElementHandle &other) const
    {
        if (!m_valid and !other.m_valid) {
            return true;
        }
        return m_valid == other.m_valid and m_iter == other.m_iter;
    }

    bool operator!=(const MeshElementHandle &other) const
    {
        return !(*this == other);
    }

    Iterator raw_iterator() const
    {
        return m_iter;
    }

};


template <typename Mesh>
struct IdentityMeshTransform
{
    static inline typename Mesh::VertexData transform_vertex(const typename Mesh::VertexData &src)
    {
        return src;
    }

    static inline typename Mesh::HalfedgeData transform_halfedge(const typename Mesh::HalfedgeData &src)
    {
        return src;
    }

    static inline typename Mesh::FaceData transform_face(const typename Mesh::FaceData &src)
    {
        return src;
    }

};


template <typename vertex_data_t = NoData,
          typename halfedge_data_t = NoData,
          typename edge_data_t = NoData,
          typename face_data_t = NoData>
class HalfedgeMesh
{
public:
    class Vertex;
    class Halfedge;
    class Edge;
    class Face;

    using VertexData = vertex_data_t;
    using HalfedgeData = halfedge_data_t;
    using EdgeData = edge_data_t;
    using FaceData = face_data_t;

    using VertexList = ffe::StableIndexVector<Vertex>;
    using HalfedgeList = ffe::StableIndexVector<Halfedge>;
    using EdgeList = ffe::StableIndexVector<Edge>;
    using FaceList = ffe::StableIndexVector<Face>;

    using VertexHandle = MeshElementHandle<typename VertexList::iterator>;
    using ConstVertexHandle = MeshElementHandle<typename VertexList::const_iterator>;
    using HalfedgeHandle = MeshElementHandle<typename HalfedgeList::iterator>;
    using ConstHalfedgeHandle = MeshElementHandle<typename HalfedgeList::const_iterator>;
    using FaceHandle = MeshElementHandle<typename FaceList::iterator>;
    using ConstFaceHandle = MeshElementHandle<typename FaceList::const_iterator>;

    class Vertex
    {
    public:
        Vertex() = default;

        template <typename... arg_ts>
        Vertex(arg_ts&&... args):
            m_data(std::forward<arg_ts>(args)...)
        {

        }

    private:
        vertex_data_t m_data;
        HalfedgeHandle m_outgoing;

    public:
        inline vertex_data_t &data()
        {
            return m_data;
        }

        inline const vertex_data_t &data() const
        {
            return m_data;
        }

        HalfedgeHandle outgoing()
        {
            return m_outgoing;
        }

        ConstHalfedgeHandle outgoing() const
        {
            return m_outgoing;
        }

        friend class HalfedgeMesh;

    };

    class Halfedge
    {
    public:
        Halfedge() = default;

        template <typename... arg_ts>
        Halfedge(arg_ts&&... args):
            m_data(std::forward<arg_ts>(args)...)
        {

        }

    private:
        halfedge_data_t m_data;
        VertexHandle m_origin;
        VertexHandle m_dest;
        HalfedgeHandle m_next;
        HalfedgeHandle m_prev;
        HalfedgeHandle m_twin;
        FaceHandle m_face;

    public:
        inline halfedge_data_t &data()
        {
            return m_data;
        }

        inline const halfedge_data_t &data() const
        {
            return m_data;
        }

        HalfedgeHandle next()
        {
            return m_next;
        }

        HalfedgeHandle prev()
        {
            return m_prev;
        }

        HalfedgeHandle twin()
        {
            return m_twin;
        }

        VertexHandle dest()
        {
            return m_dest;
        }

        VertexHandle origin()
        {
            return m_origin;
        }

        FaceHandle face()
        {
            return m_face;
        }

        friend class HalfedgeMesh;
    };

    class Edge
    {
        edge_data_t m_data;
    };

    class Face
    {
    public:
        Face() = default;

        template <typename... arg_ts>
        Face(arg_ts&&... args):
            m_data(std::forward<arg_ts>(args)...)
        {

        }

    private:
        face_data_t m_data;
        HalfedgeHandle m_first;

    public:
        face_data_t &data()
        {
            return m_data;
        }

        const face_data_t &data() const
        {
            return m_data;
        }

        HalfedgeHandle first()
        {
            return m_first;
        }

        friend class HalfedgeMesh;
    };

public:
    HalfedgeMesh() = default;
    /*HalfedgeMesh(const HalfedgeMesh &ref)
    {
        assign(ref);
    }

    HalfedgeMesh &operator=(const HalfedgeMesh &ref)
    {
        assign(ref);
        return *this;
    }*/

    HalfedgeMesh(HalfedgeMesh &&src) = default;
    HalfedgeMesh &operator=(HalfedgeMesh &&src) = default;

private:
    VertexList m_vertices;
    HalfedgeList m_halfedges;
    EdgeList m_edges;
    FaceList m_faces;

private:
    HalfedgeHandle find_edge_between(Vertex &v1, Vertex &v2)
    {
        if (!v1.outgoing() or !v2.outgoing()) {
            return nullptr;
        }

        HalfedgeHandle first = v1.outgoing();
        HalfedgeHandle curr = first;
        do {
            assert(&*curr->origin() == &v1);
            if (&(*curr->dest()) == &v2) {
                return curr;
            }
            if (curr->twin()) {
                curr = curr->twin()->next();
            } else {
                break;
            }
        } while (curr != first);
        return nullptr;
    }

    HalfedgeHandle emplace_halfedge()
    {
        return HalfedgeHandle(m_halfedges.emplace());
    }

    template <typename... arg_ts>
    FaceHandle emplace_face(arg_ts&&... args)
    {
        return FaceHandle(m_faces.emplace(std::forward<arg_ts>(args)...));
    }

public:
    template <typename... arg_ts>
    VertexHandle emplace_vertex(arg_ts&&... args)
    {
        return VertexHandle(m_vertices.emplace(std::forward<arg_ts>(args)...));
    }


    template <typename InputIt, typename... arg_ts>
    FaceHandle make_face(InputIt first, InputIt last, arg_ts&&... args)
    {
        return make_face(std::vector<VertexHandle>(first, last),
                         std::forward<arg_ts>(args)...);
    }


    template <typename... arg_ts>
    FaceHandle make_face(std::vector<VertexHandle> vertices, arg_ts&&... args)
    {
        if (vertices.size() == 1) {
            return nullptr;
        }

        std::vector<HalfedgeHandle> existing_twins;
        existing_twins.reserve(vertices.size());

        VertexHandle prev = vertices[vertices.size()-1];
        for (auto iter = vertices.begin();
             iter != vertices.end();
             ++iter)
        {
            VertexHandle curr = *iter;
            HalfedgeHandle existing = find_edge_between(*prev, *curr);
            if (existing) {
                return nullptr;
            }

            existing_twins.emplace_back(find_edge_between(*curr, *prev));
            prev = curr;
        }
        std::vector<HalfedgeHandle> new_edges;
        new_edges.reserve(vertices.size());

        FaceHandle f = emplace_face(std::forward<arg_ts>(args)...);
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            VertexHandle curr = vertices[i];
            VertexHandle prev = vertices[i > 0 ? (i-1) : (vertices.size()-1)];

            HalfedgeHandle new_halfedge = emplace_halfedge();
            const HalfedgeHandle &existing_twin = existing_twins[i];
            new_halfedge->m_twin = existing_twins[i];
            new_halfedge->m_dest = curr;
            new_halfedge->m_origin = prev;
            if (prev->outgoing()) {
                if (existing_twin) {
                    assert(!existing_twin->m_twin);
                    assert(existing_twin->m_dest == prev);
                    existing_twin->m_twin = new_halfedge;
                    new_halfedge->m_twin = existing_twin;
                }
            } else {
                assert(!existing_twin);
                prev->m_outgoing = new_halfedge;
            }
            new_halfedge->m_face = f;
            if (!new_edges.empty()) {
                new_edges.back()->m_next = new_halfedge;
                new_halfedge->m_prev = new_edges.back();
            }
            new_edges.emplace_back(new_halfedge);
        }

        f->m_first = new_edges[0];
        new_edges[0]->m_prev = new_edges[new_edges.size()-1];
        new_edges[new_edges.size()-1]->m_next = new_edges[0];

        return f;
    }

    AroundVertexIterator<HalfedgeMesh> vertices_around_vertex_begin(VertexHandle center)
    {
        return AroundVertexIterator<HalfedgeMesh>(center);
    }

    AroundVertexIterator<HalfedgeMesh> vertices_around_vertex_end(VertexHandle)
    {
        return AroundVertexIterator<HalfedgeMesh>();
    }

    FaceVertexIterator<HalfedgeMesh> face_vertices_begin(FaceHandle face)
    {
        return FaceVertexIterator<HalfedgeMesh>(face);
    }

    FaceVertexIterator<HalfedgeMesh> face_vertices_end(FaceHandle)
    {
        return FaceVertexIterator<HalfedgeMesh>();
    }

    VertexHandle vertices_begin()
    {
        return VertexHandle(m_vertices.begin());
    }

    ConstVertexHandle vertices_begin() const
    {
        return ConstVertexHandle(m_vertices.begin());
    }

    VertexHandle vertices_end()
    {
        return VertexHandle(m_vertices.end());
    }

    ConstVertexHandle vertices_end() const
    {
        return ConstVertexHandle(m_vertices.end());
    }

    std::size_t vertex_count() const
    {
        return m_vertices.size();
    }

    HalfedgeHandle halfedges_begin()
    {
        return HalfedgeHandle(m_halfedges.begin());
    }

    ConstHalfedgeHandle halfedges_begin() const
    {
        return ConstHalfedgeHandle(m_halfedges.begin());
    }

    HalfedgeHandle halfedges_end()
    {
        return HalfedgeHandle(m_halfedges.end());
    }

    ConstHalfedgeHandle halfedges_end() const
    {
        return ConstHalfedgeHandle(m_halfedges.end());
    }

    std::size_t halfedge_count() const
    {
        return m_halfedges.size();
    }

    FaceHandle faces_begin()
    {
        return FaceHandle(m_faces.begin());
    }

    ConstFaceHandle faces_begin() const
    {
        return ConstFaceHandle(m_faces.begin());
    }

    FaceHandle faces_end()
    {
        return FaceHandle(m_faces.end());
    }

    ConstFaceHandle faces_end() const
    {
        return ConstFaceHandle(m_faces.end());
    }

    std::size_t face_count() const
    {
        return m_faces.size();
    }

    void clear()
    {
        m_vertices.clear();
        m_halfedges.clear();
        m_edges.clear();
        m_faces.clear();
    }

    /*void assign(const HalfedgeMesh &src)
    {
        clear();

    }*/

    /*template <typename Mesh, typename MeshTransform>
    void assign(const Mesh &mesh, MeshTransform &&transform)
    {
        clear();
        m_vertices = mesh.m_vertices;
        m_halfedges.resize(mesh.halfedge_count());
        m_faces.resize(mesh.face_count());

        std::vector<VertexHandle> my_vertices;
        std::vector<HalfedgeHandle> my_halfedges;
        std::vector<FaceHandle> my_faces;

        std::vector<typename Mesh::ConstVertexHandle> src_vertices;
        std::vector<typename Mesh::ConstHalfedgeHandle> src_halfedges;
        std::vector<typename Mesh::ConstFaceHandle> src_faces;

        my_vertices.reserve(mesh.vertex_count());
        src_vertices.reserve(mesh.vertex_count());

        my_halfedges.reserve(mesh.halfedge_count());
        src_halfedges.reserve(mesh.halfedge_count());

        my_faces.reserve(mesh.face_count());
        src_faces.reserve(mesh.face_count());

        {
            auto my_iter = m_vertices.begin();
            auto src_iter = mesh.vertices_begin();
            while (my_iter != m_vertices.end())
            {
                my_vertices.emplace_back(VertexHandle(my_iter));
                src_vertices.emplace_back(typename Mesh::ConstVertexHandle(src_iter));

                my_iter->m_data = transform.transform_vertex(src_iter->data());
                if (src_iter->outgoing()) {
                    my_iter->m_outgoing = m_vertices.iterator_from_index(src_iter->outgoing()->raw_index());
                }

                my_iter++;
                src_iter++;
            }
        }
        {
            auto my_iter = m_halfedges.begin();
            auto src_iter = mesh.halfedges_begin();
            while (my_iter != m_halfedges.end())
            {
                my_halfedges.emplace_back(HalfedgeHandle(my_iter));
                src_halfedges.emplace_back(typename Mesh::ConstHalfedgeHandle(src_iter));

                my_iter->m_data = transform.transform_halfedge(src_iter->data());

                my_iter++;
                src_iter++;
            }
        }
        {
            auto my_iter = m_faces.begin();
            auto src_iter = mesh.faces_begin();
            while (my_iter != m_faces.end())
            {
                my_faces.emplace_back(FaceHandle(my_iter));
                src_faces.emplace_back(typename Mesh::ConstFaceHandle(src_iter));

                my_iter->m_data = transform.transform_face(src_iter->data());

                my_iter++;
                src_iter++;
            }
        }
    }*/

};

#include <ostream>

template <typename Iterator>
std::ostream &operator<<(std::ostream &stream, const MeshElementHandle<Iterator> &h)
{
    if (h) {
        return stream << "MeshElementHandle(" << &(*h) << ")";
    } else {
        return stream << "MeshElementHandle(nullptr)";
    }
}

namespace std {

template <typename Iterator>
struct hash<::MeshElementHandle<Iterator> >
{
private:
    using SubHash = std::hash<ffe::StableIndexVectorBase::RawIndex>;

public:
    using result_type = typename SubHash::result_type;
    using argument_type = ::MeshElementHandle<Iterator>;

private:
    SubHash m_hash;

public:
    inline result_type operator()(const argument_type &handle) const
    {
        return m_hash(handle.raw_iterator().raw_index());
    }

};

}

#endif
