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

#include "ffengine/sim/objects.hpp"

#include <iostream>


class MyObject: public sim::Object
{
public:
    explicit MyObject(sim::Object::ID object_id):
        sim::Object(object_id)
    {

    }

    MyObject(sim::Object::ID object_id,
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
    OtherObject(sim::Object::ID object_id):
        sim::Object(object_id)
    {
        throw std::runtime_error("throw a thing");
    }

};


const sim::Object::ID NULL_OBJECT_ID = sim::Object::NULL_OBJECT_ID;


TEST_CASE("sim/ObjectManager/allocate")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    CHECK(obj1.object_id() == 1);
    MyObject &obj2 = om.allocate<MyObject>();
    CHECK(obj2.object_id() == 2);
    MyObject &obj3 = om.allocate<MyObject>();
    CHECK(obj3.object_id() == 3);
}

TEST_CASE("sim/ObjectManager/get")
{
    sim::ObjectManager om;
    om.allocate<MyObject>(10);
    om.allocate<MyObject>(20);
    om.allocate<MyObject>(30);

    {
        MyObject *obj = om.get_safe<MyObject>(2);
        REQUIRE(obj != nullptr);
        CHECK(obj->m_value == 20);
    }

    {
        OtherObject *obj = om.get_safe<OtherObject>(2);
        CHECK(obj == nullptr);
    }
}

TEST_CASE("sim/ObjectManager/kill")
{
    sim::ObjectManager om;
    om.allocate<MyObject>(10);
    om.allocate<MyObject>(20);
    om.allocate<MyObject>(30);

    om.kill(1);
    om.kill(3);

    MyObject &obj = om.allocate<MyObject>(40);
    CHECK(obj.object_id() == 1);

    om.kill(1);
    om.kill(2);

    CHECK(om.allocate<MyObject>(100).object_id() == 1);
    CHECK(om.allocate<MyObject>(200).object_id() == 2);
    CHECK(om.allocate<MyObject>(300).object_id() == 3);
}

TEST_CASE("sim/ObjectManager/continous_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 1; i <= 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
        CHECK(obj.object_id() == i);
    }
}

TEST_CASE("sim/ObjectManager/random_dealloc_and_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 1; i <= 10000; i++) {
        MyObject &obj = om.allocate<MyObject>(i*10);
    }

    for (unsigned int i = 1; i <= 10000; i += 2) {
        om.kill(i);
    }

    for (unsigned int i = 2; i <= 10000; i += 2) {
        MyObject *obj = om.get_safe<MyObject>(i);
        REQUIRE(obj);
        CHECK(obj->object_id() == i);
        CHECK(obj->m_value == i*10);
    }
}

TEST_CASE("sim/ObjectManager/reverse_dealloc_forward_alloc")
{
    sim::ObjectManager om;
    for (unsigned int i = 1; i <= 10000; i++) {
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
    for (unsigned int i = 1; i <= 10000; i++) {
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
    CHECK(om.allocate<MyObject>().object_id() == 1);
}

TEST_CASE("sim/ObjectManager/emplace")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    MyObject &obj2 = om.emplace<MyObject>(3);

    // make sure the objects still exist
    REQUIRE(om.get_safe<MyObject>(1));
    REQUIRE(om.get_safe<MyObject>(3));

    CHECK(obj1.object_id() == 1);
    CHECK(obj2.object_id() == 3);

    CHECK(om.allocate<MyObject>().object_id() == 2);
    CHECK(om.allocate<MyObject>().object_id() == 4);
}

TEST_CASE("sim/ObjectManager/emplace/freelist_case_beginning")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    MyObject &obj2 = om.emplace<MyObject>(2);

    // make sure the objects still exist
    REQUIRE(om.get_safe<MyObject>(1));
    REQUIRE(om.get_safe<MyObject>(2));

    CHECK(obj1.object_id() == 1);
    CHECK(obj2.object_id() == 2);

    CHECK(om.allocate<MyObject>().object_id() == 3);
    CHECK(om.allocate<MyObject>().object_id() == 4);
}

TEST_CASE("sim/ObjectManager/emplace/freelist_case_end")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    MyObject &obj2 = om.emplace<MyObject>(4);
    MyObject &obj3 = om.emplace<MyObject>(3);

    // make sure the objects still exist
    REQUIRE(om.get_safe<MyObject>(1));
    REQUIRE(om.get_safe<MyObject>(4));
    REQUIRE(om.get_safe<MyObject>(3));

    CHECK(obj1.object_id() == 1);
    CHECK(obj2.object_id() == 4);
    CHECK(obj3.object_id() == 3);

    CHECK(om.allocate<MyObject>().object_id() == 2);
    CHECK(om.allocate<MyObject>().object_id() == 5);
}

TEST_CASE("sim/ObjectManager/emplace/conflict")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    CHECK_THROWS(om.emplace<MyObject>(obj1.object_id()));
}

TEST_CASE("sim/ObjectManager/emplace/null_id_allocates_new_id")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    MyObject &obj2 = om.emplace<MyObject>(sim::Object::NULL_OBJECT_ID);
    CHECK(obj2.object_id() == 2);
}

TEST_CASE("sim/ObjectManager/share")
{
    sim::ObjectManager om;
    MyObject &obj1 = om.allocate<MyObject>();
    sim::Object::ID id = obj1.object_id();
    auto ptr = om.share(obj1);
    CHECK(ptr.get() == &obj1);
    CHECK(ptr);
    CHECK(ptr.object_id() == id);
    CHECK(ptr.was_valid());
    om.kill(obj1);
    CHECK_FALSE(ptr);
    CHECK(ptr.get() == nullptr);
    CHECK(ptr.was_valid());
    CHECK(ptr.object_id() == id);
}


TEST_CASE("sim/object_ptr/default_constructor")
{
    sim::object_ptr<MyObject> ptr;
    CHECK_FALSE(ptr);
    CHECK_FALSE(ptr.was_valid());
    CHECK(ptr.get() == nullptr);
    CHECK(ptr.object_id() == NULL_OBJECT_ID);
}

TEST_CASE("sim/object_ptr/nullptr_constructor")
{
    sim::object_ptr<MyObject> ptr(nullptr);
    CHECK_FALSE(ptr);
    CHECK_FALSE(ptr.was_valid());
    CHECK(ptr.get() == nullptr);
    CHECK(ptr.object_id() == NULL_OBJECT_ID);
}

TEST_CASE("sim/object_ptr/reference_constructor")
{
    MyObject obj(123);
    sim::object_ptr<MyObject> ptr(obj);
    CHECK(ptr);
    CHECK(ptr.was_valid());
    CHECK(ptr.get() == &obj);
    CHECK(ptr.object_id() == obj.object_id());
}

TEST_CASE("sim/object_ptr/move_constructor")
{
    MyObject obj(123);
    sim::object_ptr<MyObject> ptr2(obj);
    sim::object_ptr<MyObject> ptr1(std::move(ptr2));
    CHECK_FALSE(ptr2);
    CHECK_FALSE(ptr2.was_valid());
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
}

TEST_CASE("sim/object_ptr/copy_constructor")
{
    MyObject obj(123);
    sim::object_ptr<MyObject> ptr2(obj);
    sim::object_ptr<MyObject> ptr1(ptr2);
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
    CHECK(ptr2);
    CHECK(ptr2.was_valid());
    CHECK(ptr2.get() == &obj);
}

TEST_CASE("sim/object_ptr/move_assignment")
{
    MyObject obj(123);
    sim::object_ptr<MyObject> ptr1(nullptr);
    sim::object_ptr<MyObject> ptr2(obj);
    ptr1 = std::move(ptr2);
    CHECK_FALSE(ptr2);
    CHECK_FALSE(ptr2.was_valid());
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
}

TEST_CASE("sim/object_ptr/copy_assignment")
{
    MyObject obj(123);
    sim::object_ptr<MyObject> ptr1(nullptr);
    sim::object_ptr<MyObject> ptr2(obj);
    ptr1 = ptr2;
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
    CHECK(ptr2);
    CHECK(ptr2.was_valid());
    CHECK(ptr2.get() == &obj);
}

TEST_CASE("sim/object_ptr/upcast")
{
    MyObject obj(123);
    sim::object_ptr<sim::Object> ptr1(nullptr);
    sim::object_ptr<MyObject> ptr2(obj);
    ptr1 = std::move(ptr2);
    CHECK_FALSE(ptr2);
    CHECK_FALSE(ptr2.was_valid());
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
}

TEST_CASE("sim/object_ptr/static_downcast")
{
    MyObject obj(123);
    sim::object_ptr<sim::Object> ptr1(obj);
    sim::object_ptr<MyObject> ptr2(sim::static_object_cast<MyObject>(std::move(ptr1)));
    CHECK_FALSE(ptr1);
    CHECK_FALSE(ptr1.was_valid());
    CHECK(ptr2);
    CHECK(ptr2.was_valid());
    CHECK(ptr2.get() == &obj);
}

TEST_CASE("sim/object_ptr/static_downcast_success")
{
    MyObject obj(123);
    sim::object_ptr<sim::Object> ptr1(obj);
    sim::object_ptr<MyObject> ptr2(sim::dynamic_object_cast<MyObject>(std::move(ptr1)));
    CHECK_FALSE(ptr1);
    CHECK_FALSE(ptr1.was_valid());
    CHECK(ptr2);
    CHECK(ptr2.was_valid());
    CHECK(ptr2.get() == &obj);
}

TEST_CASE("sim/object_ptr/dynamic_downcast_failure")
{
    MyObject obj(123);
    sim::object_ptr<sim::Object> ptr1(obj);
    sim::object_ptr<OtherObject> ptr2(sim::dynamic_object_cast<OtherObject>(std::move(ptr1)));
    CHECK(ptr1);
    CHECK(ptr1.was_valid());
    CHECK(ptr1.get() == &obj);
    CHECK_FALSE(ptr2);
    CHECK_FALSE(ptr2.was_valid());
}

