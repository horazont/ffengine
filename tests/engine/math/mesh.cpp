#include <catch.hpp>

#include <vector>

#include "ffengine/math/mesh.hpp"


struct VertexData
{
    VertexData() = default;

    VertexData(int n):
        n(n)
    {

    }

    int n;
};


using TestMesh = HalfedgeMesh<VertexData>;


#define DECL_TEST_MESH(name) TestMesh name; const TestMesh &const_##name = name;


TEST_CASE("math/mesh/HalfedgeMesh::VertexHandle/false_by_default")
{
    TestMesh::VertexHandle h;
    TestMesh::VertexHandle h2 = h;
    CHECK_FALSE(h);
    CHECK_FALSE(h2);
}


TEST_CASE("math/mesh/HalfedgeMesh")
{
    DECL_TEST_MESH(mesh);

    CHECK(mesh.vertex_count() == 0);
    CHECK(const_mesh.vertex_count() == 0);
}


TEST_CASE("math/mesh/HalfedgeMesh/emplace_vertex")
{
    DECL_TEST_MESH(mesh);

    TestMesh::VertexHandle h = mesh.emplace_vertex(10);
    CHECK(mesh.vertex_count() == 1);
    CHECK(const_mesh.vertex_count() == 1);
    CHECK(h->data().n == 10);
}


TEST_CASE("math/mesh/HalfedgeMesh/make_face")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3);

    std::vector<TestMesh::VertexHandle> buf({v1, v2, v3});
    auto f1 = mesh.make_face(buf);
    REQUIRE(f1);

    REQUIRE(v1->outgoing());
    CHECK(v1->outgoing()->dest() == v2);
    CHECK(v1->outgoing()->face() == f1);
    CHECK_FALSE(v1->outgoing()->twin());
    REQUIRE(v2->outgoing());
    CHECK(v2->outgoing()->dest() == v3);
    CHECK(v2->outgoing()->face() == f1);
    CHECK_FALSE(v2->outgoing()->twin());
    REQUIRE(v3->outgoing());
    CHECK(v3->outgoing()->dest() == v1);
    CHECK(v3->outgoing()->face() == f1);
    CHECK_FALSE(v3->outgoing()->twin());

    buf.clear();
    buf.assign({v3, v2, v4});
    auto f2 = mesh.make_face(buf.begin(), buf.end());
    CHECK(f2);

    REQUIRE(v2->outgoing());
    CHECK(v2->outgoing()->twin());
    CHECK(v2->outgoing()->twin()->twin() == v2->outgoing());
    CHECK(v2->outgoing()->twin()->origin() == v3);
    CHECK_FALSE(v1->outgoing()->twin());
    CHECK_FALSE(v3->outgoing()->twin());

    REQUIRE(v4->outgoing());
    CHECK(v4->outgoing()->dest() == v3);

    std::vector<TestMesh::VertexHandle> dest;
    std::vector<TestMesh::VertexHandle> check({v3, v4, v1});
    dest.assign(mesh.vertices_around_vertex_begin(v2),
                mesh.vertices_around_vertex_end(v2));
    CHECK(dest == check);

    check.assign({v1, v4, v2});
    dest.assign(mesh.vertices_around_vertex_begin(v3),
                mesh.vertices_around_vertex_end(v3));
    CHECK(dest == check);

    check.assign({v1, v2, v3});
    dest.assign(mesh.face_vertices_begin(f1), mesh.face_vertices_end(f1));
    CHECK(dest == check);

    check.assign({v3, v2, v4});
    dest.assign(mesh.face_vertices_begin(f2), mesh.face_vertices_end(f2));
    CHECK(dest == check);
}


TEST_CASE("math/mesh/HalfedgeMesh/make_face/triangle_fan")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3),
            v5 = mesh.emplace_vertex(4),
            v6 = mesh.emplace_vertex(5);

    auto
            f1 = mesh.make_face({v1, v2, v3}),
            f2 = mesh.make_face({v1, v3, v4}),
            f3 = mesh.make_face({v1, v4, v5}),
            f4 = mesh.make_face({v1, v5, v6}),
            f5 = mesh.make_face({v1, v6, v2});

    CHECK(f1);
    CHECK(f2);
    CHECK(f3);
    CHECK(f4);
    CHECK(f5);

    std::vector<TestMesh::VertexHandle> dest(mesh.vertices_around_vertex_begin(v1),
                                             mesh.vertices_around_vertex_end(v1));
    std::vector<TestMesh::VertexHandle> check({v2, v6, v5, v4, v3});
    CHECK(dest == check);
}


TEST_CASE("math/mesh/HalfedgeMesh/clear")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3),
            v5 = mesh.emplace_vertex(4),
            v6 = mesh.emplace_vertex(5);

    mesh.make_face({v1, v2, v3});
    mesh.make_face({v1, v3, v4});
    mesh.make_face({v1, v4, v5});
    mesh.make_face({v1, v5, v6});
    mesh.make_face({v1, v6, v2});

    CHECK(mesh.vertex_count() == 6);
    CHECK(mesh.face_count() == 5);

    mesh.clear();

    CHECK(mesh.vertex_count() == 0);
    CHECK(mesh.face_count() == 0);
}


TEST_CASE("math/mesh/HalfedgeMesh/HalfedgeMesh(HalfedgeMesh&&)")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3),
            v5 = mesh.emplace_vertex(4),
            v6 = mesh.emplace_vertex(5);

    mesh.make_face({v1, v2, v3});
    mesh.make_face({v1, v3, v4});
    mesh.make_face({v1, v4, v5});
    mesh.make_face({v1, v5, v6});
    mesh.make_face({v1, v6, v2});

    CHECK(mesh.vertex_count() == 6);
    CHECK(mesh.face_count() == 5);

    TestMesh dest(std::move(mesh));

    CHECK(mesh.vertex_count() == 0);
    CHECK(mesh.face_count() == 0);

    CHECK(dest.vertex_count() == 6);
    CHECK(dest.face_count() == 5);
}


TEST_CASE("math/mesh/HalfedgeMesh/HalfedgeMesh &operator=(HalfedgeMesh&&)")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3),
            v5 = mesh.emplace_vertex(4),
            v6 = mesh.emplace_vertex(5);

    mesh.make_face({v1, v2, v3});
    mesh.make_face({v1, v3, v4});
    mesh.make_face({v1, v4, v5});
    mesh.make_face({v1, v5, v6});
    mesh.make_face({v1, v6, v2});

    CHECK(mesh.vertex_count() == 6);
    CHECK(mesh.face_count() == 5);

    TestMesh dest;
    dest = std::move(mesh);

    CHECK(mesh.vertex_count() == 0);
    CHECK(mesh.face_count() == 0);

    CHECK(dest.vertex_count() == 6);
    CHECK(dest.face_count() == 5);

    CHECK(v4->data().n == 3);
}


/*TEST_CASE("math/mesh/HalfedgeMesh/HalfedgeMesh(const HalfedgeMesh&)")
{
    DECL_TEST_MESH(mesh);

    auto
            v1 = mesh.emplace_vertex(0),
            v2 = mesh.emplace_vertex(1),
            v3 = mesh.emplace_vertex(2),
            v4 = mesh.emplace_vertex(3),
            v5 = mesh.emplace_vertex(4),
            v6 = mesh.emplace_vertex(5);

    mesh.make_face({v1, v2, v3});
    mesh.make_face({v1, v3, v4});
    mesh.make_face({v1, v4, v5});
    mesh.make_face({v1, v5, v6});
    mesh.make_face({v1, v6, v2});

    CHECK(mesh.vertex_count() == 6);
    CHECK(mesh.face_count() == 5);

    TestMesh dest(mesh);

    CHECK(mesh.vertex_count() == 6);
    CHECK(mesh.face_count() == 5);

    CHECK(dest.vertex_count() == 6);
    CHECK(dest.face_count() == 5);

    CHECK(mesh.vertices_begin() == v1);
    CHECK(dest.vertices_begin() != v1);

    v1->data().n = 10;
    CHECK(dest.vertices_begin()->data().n == 0);

    std::vector<TestMesh::VertexHandle> circle(
                dest.vertices_around_vertex_begin(dest.vertices_begin()),
                dest.vertices_around_vertex_end(dest.vertices_begin()));
    CHECK(circle.size() == 5);

}*/
