/**********************************************************************
File name: objects.cpp
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

#include "engine/sim/objects.hpp"


class MyObject: public sim::Object
{
public:
    explicit MyObject(sim::ObjectID object_id):
        sim::Object(object_id)
    {

    }

    MyObject(sim::ObjectID object_id,
             unsigned int value):
        sim::Object(object_id),
        m_value(value)
    {

    }

    unsigned int m_value;
};


class OtherObject: public sim::Object
{
public:
    OtherObject(sim::ObjectID object_id):
        sim::Object(object_id)
    {
        throw std::runtime_error("throw a thing");
    }

};


TEST_CASE("sim/ObjectManager/allocate")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    CHECK(obj1.object_id() == 0);
    MyObject &obj2 = om.allocate<MyObject>();
    CHECK(obj2.object_id() == 1);
    MyObject &obj3 = om.allocate<MyObject>();
    CHECK(obj3.object_id() == 2);
}

TEST_CASE("sim/ObjectManager/get")
{
    sim::ObjectManager om;
    om.allocate<MyObject>(10);
    om.allocate<MyObject>(20);
    om.allocate<MyObject>(30);

    {
        MyObject *obj = om.get_safe<MyObject>(1);
        REQUIRE(obj != nullptr);
        CHECK(obj->m_value == 20);
    }

    {
        OtherObject *obj = om.get_safe<OtherObject>(1);
        CHECK(obj == nullptr);
    }
}

TEST_CASE("sim/ObjectManager/kill")
{
    sim::ObjectManager om;
    om.allocate<MyObject>(10);
    om.allocate<MyObject>(20);
    om.allocate<MyObject>(30);

    om.kill(0);
    om.kill(2);

    MyObject &obj = om.allocate<MyObject>(40);
    CHECK(obj.object_id() == 0);

    om.kill(0);
    om.kill(1);

    CHECK(om.allocate<MyObject>(100).object_id() == 0);
    CHECK(om.allocate<MyObject>(200).object_id() == 1);
    CHECK(om.allocate<MyObject>(300).object_id() == 2);
}

TEST_CASE("sim/ObjectManager/continous_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 0; i < 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
        CHECK(obj.object_id() == i);
    }
}

TEST_CASE("sim/ObjectManager/random_dealloc_and_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 0; i < 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
    }

    for (unsigned int i = 0; i < 10000; i += 2) {
        om.kill(i);
    }

    for (unsigned int i = 1; i < 10000; i += 2) {
        MyObject *obj = om.get_safe<MyObject>(i);
        REQUIRE(obj);
        CHECK(obj->m_value == i*10);
    }
}

TEST_CASE("sim/ObjectManager/reverse_dealloc_forward_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 0; i < 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
    }

    // check that reallocation of IDs works if they have been deallocated in
    // reverse
    for (unsigned int i = 100; i > 90; i--) {
        om.kill(i);
    }

    for (unsigned int i = 91; i <= 100; i++) {
        CHECK(om.allocate<MyObject>().object_id() == i);
    }
}

TEST_CASE("sim/ObjectManager/forward_dealloc_forward_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 0; i < 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
    }

    // check that reallocation of IDs works if they have been deallocated in
    // reverse
    for (unsigned int i = 90; i < 100; i++) {
        om.kill(i);
    }

    for (unsigned int i = 90; i < 100; i++) {
        CHECK(om.allocate<MyObject>().object_id() == i);
    }
}

TEST_CASE("sim/ObjectManager/alloc_exception_handling")
{
    sim::ObjectManager om;
    CHECK_THROWS(om.allocate<OtherObject>());
    CHECK(om.allocate<MyObject>().object_id() == 0);
}

