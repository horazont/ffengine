/**********************************************************************
File name: octree.cpp
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
#include <catch.hpp>

#include "ffengine/math/octree.hpp"

#include <iostream>


class TestObject: public ffe::OctreeObject
{
public:
    void set_bounding_sphere(const Sphere &sph)
    {
        update_bounds(sph);
    }
};


TEST_CASE("math/octree/Octree/insert_object")
{
    TestObject obj;
    obj.set_bounding_sphere(Sphere{Vector3f(-1, 0, 0), 0.5f});
    ffe::Octree tree;
    ffe::OctreeNode *node = tree.insert_object(&obj);
    REQUIRE(node);
    CHECK(node->bounds().min == Vector3f(-1.5, -0.5, -0.5));
    CHECK(node->bounds().max == Vector3f(-0.5, 0.5, 0.5));
}

TEST_CASE("math/octree/Octree/auto_remove_object_on_deletion_of_object")
{
    std::unique_ptr<TestObject> obj(new TestObject());
    ffe::Octree tree;
    ffe::OctreeNode *node = tree.insert_object(obj.get());
    CHECK(!node->bounds().empty());
    obj = nullptr;
    CHECK(node->bounds().empty());
}

TEST_CASE("math/octree/Octree/auto_disassociate_object_on_deletion")
{
    TestObject obj;
    {
        ffe::Octree tree;
        ffe::OctreeNode *node = tree.insert_object(&obj);
        CHECK(obj.octree() == &tree);
    }
    CHECK(!obj.octree());
}

TEST_CASE("math/octree/Octree/insert_object/autosplit")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, -1);
    coords.emplace_back(-1, -1, 1);
    coords.emplace_back(-1, 1, -1);
    coords.emplace_back(-1, 1, 1);
    coords.emplace_back(1, -1, -1);
    coords.emplace_back(1, -1, 1);
    coords.emplace_back(1, 1, -1);
    coords.emplace_back(1, 1, 1);

    std::vector<AABB> expected_bounds;
    expected_bounds.emplace_back(Vector3f(-1.3, -1.3, -1.3), Vector3f(-0.7, -0.7, -0.7));
    expected_bounds.emplace_back(Vector3f(-1.3, -1.3, 0.7), Vector3f(-0.7, -0.7, 1.3));
    expected_bounds.emplace_back(Vector3f(-1.3, 0.7, -1.3), Vector3f(-0.7, 1.3, -0.7));
    expected_bounds.emplace_back(Vector3f(-1.3, 0.7, 0.7), Vector3f(-0.7, 1.3, 1.3));
    expected_bounds.emplace_back(Vector3f(0.7, -1.3, -1.3), Vector3f(1.3, -0.7, -0.7));
    expected_bounds.emplace_back(Vector3f(0.7, -1.3, 0.7), Vector3f(1.3, -0.7, 1.3));
    expected_bounds.emplace_back(Vector3f(0.7, 0.7, -1.3), Vector3f(1.3, 1.3, -0.7));
    expected_bounds.emplace_back(Vector3f(0.7, 0.7, 0.7), Vector3f(1.3, 1.3, 1.3));

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }

    CHECK(tree.root().is_split());
    ffe::OctreeNode &root = tree.root();
    CHECK(root.bounds() ==
          AABB(Vector3f(-1.3, -1.3, -1.3), Vector3f(1.3, 1.3, 1.3)));

    for (unsigned int i = 0; i < 16; ++i)
    {
        unsigned int child_index = i % 8;
        TestObject &obj = *objects[i];
        ffe::OctreeNode *child = root.child(child_index);
        REQUIRE(child);
        CHECK(std::find(child->cbegin(), child->cend(), &obj) != child->cend());
    }

    for (unsigned int i = 0; i < 8; ++i)
    {
        CHECK(root.child(i)->bounds() == expected_bounds[i]);
    }
}

TEST_CASE("math/octree/Octree/remove_object/remerge")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, 0);
    coords.emplace_back(-1, -1, 0);
    coords.emplace_back(-1, 1, 0);
    coords.emplace_back(-1, 1, 0);
    coords.emplace_back(1, -1, 0);
    coords.emplace_back(1, -1, 0);
    coords.emplace_back(1, 1, 0);
    coords.emplace_back(1, 1, 0);

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }

    CHECK(tree.root().is_split());

    objects.clear();

    CHECK(!tree.root().is_split());
}

TEST_CASE("math/octree/Octree/split/degrade_to_quadtree")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, 0);
    coords.emplace_back(-1, 1, 0);
    coords.emplace_back(1, -1, 0);
    coords.emplace_back(1, 1, 0);
    coords.emplace_back(-1, -1, 0);
    coords.emplace_back(-1, 1, 0);
    coords.emplace_back(1, -1, 0);
    coords.emplace_back(1, 1, 0);

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }

    CHECK(tree.root().is_split());
    ffe::OctreeNode &root = tree.root();

    for (unsigned int i = 0; i < 16; ++i)
    {
        // XXX: we’re testing an implementation detail here by using the
        // indices; if tests start to fail here, the bit <-> plane association
        // might have changed
        unsigned int child_index = (i % 4) << 1;
        TestObject &obj = *objects[i];
        ffe::OctreeNode *child = root.child(child_index);
        REQUIRE(child);
        CHECK(std::find(child->cbegin(), child->cend(), &obj) != child->cend());
    }
}

TEST_CASE("math/octree/Octree/remove_object/parent_auto_resplit")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, -1);
    coords.emplace_back(-1, -1, 1);
    coords.emplace_back(-1, 1, -1);
    coords.emplace_back(-1, 1, 1);
    coords.emplace_back(1, -1, -1);
    coords.emplace_back(1, -1, 1);
    coords.emplace_back(1, 1, -1);
    coords.emplace_back(1, 1, 1);

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }


    CHECK(tree.root().is_split());

    // this is the first split; we now add a bunch of objects which are on the
    // current splitting planes and remove the old objects. this should trigger
    // a re-split

    {
        std::vector<std::unique_ptr<TestObject> > new_objects;
        std::vector<Vector3f> coords;
        coords.emplace_back(-1, -1, 0);
        coords.emplace_back(-1, 1, 0);
        coords.emplace_back(1, -1, 0);
        coords.emplace_back(1, 1, 0);
        coords.emplace_back(-1, -1, 0);
        coords.emplace_back(-1, 1, 0);
        coords.emplace_back(1, -1, 0);
        coords.emplace_back(1, 1, 0);

        for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
        {
            for (auto &coord: coords)
            {
                auto obj = std::make_unique<TestObject>();
                obj->set_bounding_sphere(Sphere{coord, radius});
                tree.insert_object(obj.get());
                new_objects.emplace_back(std::move(obj));
            }
        }

        // swap old and new object vectors
        std::swap(objects, new_objects);
        // the new_objects vector now holds the old objects and goes out of
        // scope, thereby removing the objects from the Octree
    }

    CHECK(tree.root().is_split());
    ffe::OctreeNode &root = tree.root();

    for (unsigned int i = 0; i < 16; ++i)
    {
        // XXX: we’re testing an implementation detail here by using the
        // indices; if tests start to fail here, the bit <-> plane association
        // might have changed
        unsigned int child_index = (i % 4) << 1;
        TestObject &obj = *objects[i];
        ffe::OctreeNode *child = root.child(child_index);
        REQUIRE(child);
        CHECK(std::find(child->cbegin(), child->cend(), &obj) != child->cend());
    }

}

TEST_CASE("math/octree/Octree/select_nodes_by_ray")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, -1);
    coords.emplace_back(-1, -1, 1);
    coords.emplace_back(-1, 1, -1);
    coords.emplace_back(-1, 1, 1);
    coords.emplace_back(1, -1, -1);
    coords.emplace_back(1, -1, 1);
    coords.emplace_back(1, 1, -1);
    coords.emplace_back(1, 1, 1);

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }

    Ray r(Vector3f(-1, -1.25, 2), Vector3f(0, 0, -1));

    // selected objects shall be ordered by ray hit order
    std::vector<ffe::OctreeNode*> expected_nodes;
    expected_nodes.push_back(tree.root().child(0b000));
    expected_nodes.push_back(tree.root().child(0b001));

    std::vector<ffe::OctreeRayHitInfo> hitset;

    tree.select_nodes_by_ray(r, hitset);

    std::vector<ffe::OctreeNode*> selected_nodes;

    for (auto &entry: hitset)
    {
        selected_nodes.push_back(entry.node);
    }

    CHECK(selected_nodes == expected_nodes);
}

TEST_CASE("math/octree/Octree/select_nodes_by_frustum")
{
    std::vector<Vector3f> coords;
    coords.emplace_back(-1, -1, -1);
    coords.emplace_back(-1, -1, 1);
    coords.emplace_back(-1, 1, -1);
    coords.emplace_back(-1, 1, 1);
    coords.emplace_back(1, -1, -1);
    coords.emplace_back(1, -1, 1);
    coords.emplace_back(1, 1, -1);
    coords.emplace_back(1, 1, 1);

    ffe::Octree tree;

    CHECK(!tree.root().is_split());

    std::vector<std::unique_ptr<TestObject> > objects;
    for (float radius = 0.1f; radius < 0.35f; radius += 0.2f)
    {
        for (auto &coord: coords)
        {
            auto obj = std::make_unique<TestObject>();
            obj->set_bounding_sphere(Sphere{coord, radius});
            tree.insert_object(obj.get());
            objects.emplace_back(std::move(obj));
        }
    }

    // insert an object into the root to test that nodes with children and
    // objects are selected
    auto obj = std::make_unique<TestObject>();
    obj->set_bounding_sphere(Sphere{Vector3f(0, 0, 0), 0.1f});
    tree.insert_object(obj.get());
    objects.emplace_back(std::move(obj));

    // construct the planes so that we select the positive X nodes
    std::array<Plane, 6> frustum({{
                                      Plane(Vector4f(1, 0, 0, 0.9)),
                                      Plane(Vector4f(-1, 0, 0, -1)),
                                      Plane(Vector4f(0, 1, 0, -1)),
                                      Plane(Vector4f(0, -1, 0, -1)),
                                      Plane(Vector4f(0, 0, 1, -1)),
                                      Plane(Vector4f(0, 0, -1, -1)),
                                  }});


    std::vector<ffe::OctreeNode*> expected_nodes;
    expected_nodes.push_back(&tree.root());
    expected_nodes.push_back(tree.root().child(0b100));
    expected_nodes.push_back(tree.root().child(0b101));
    expected_nodes.push_back(tree.root().child(0b110));
    expected_nodes.push_back(tree.root().child(0b111));

    std::vector<ffe::OctreeNode*> hitset;

    tree.select_nodes_by_frustum(frustum, hitset);

    CHECK(hitset == expected_nodes);
}
