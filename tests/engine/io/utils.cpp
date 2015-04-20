/**********************************************************************
File name: utils.cpp
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

#include "engine/io/utils.hpp"

#include <iostream>

using namespace io;


TEST_CASE("io/utils/absolutify/keeps_absolute_paths",
          "absolutify must not change absolute paths")
{
    std::string path = "/some/test/path";
    CHECK(path == absolutify(path));
}

TEST_CASE("io/utils/absolutify/dot",
          "absolutify must remove single `.' elements")
{
    std::string path_abs = "/some/test/path";
    std::string path_rel = "/./some/./test/./path";
    CHECK(path_abs == absolutify(path_rel));
}

TEST_CASE("io/utils/absolutify/ddot",
          "absolutify must properly deal with `..' elements")
{
    std::string path_abs = "/some/other/test/path";
    std::string path_rel = "/some/test/../other/test/../test/path";
    CHECK(path_abs == absolutify(path_rel));
}

TEST_CASE("io/utils/absolutify/single_file",
          "absolutify must absolutify a single file")
{
    std::string path = "test.txt";
    std::string path_norm = "/test.txt";
    CHECK(path_norm == absolutify(path));
}

TEST_CASE("io/utils/basename/only_basename",
          "basename must find the basename of a file without path")
{
    std::string path = "test.txt";
    std::string basenamed = "test.txt";
    CHECK(basenamed == basename(path));
}

TEST_CASE("io/utils/basename/long_path",
          "basename must find the basename of a usual long path")
{
    std::string path = "/foo/bar/baz/test.txt";
    std::string basenamed = "test.txt";
    CHECK(basenamed == basename(path));
}

TEST_CASE("io/utils/dirname/only_basename",
          "dirname must return empty result for a file without path")
{
    std::string path = "test.txt";
    std::string dirnamed = "";
    CHECK(dirnamed == dirname(path));
}

TEST_CASE("io/utils/dirname/long_path",
          "dirname must find the dirname of a usual long path")
{
    std::string path = "/foo/bar/baz/test.txt";
    std::string dirnamed = "/foo/bar/baz";
    CHECK(dirnamed == dirname(path));
}

TEST_CASE("io/utils/normalize_vfs_path/keeps_normalized_paths",
          "normalize_vfs_path must keep absolute paths")
{
    std::string path = "/some/normalized/path";
    CHECK(path == normalize_vfs_path(path));
}

TEST_CASE("io/utils/normalize_vfs_path/remove_trailing",
          "normalize_vfs_path must remove trailing slashes")
{
    std::string path = "/with/trailing/path/";
    std::string path_norm = "/with/trailing/path";
    CHECK(path_norm == normalize_vfs_path(path));
}

TEST_CASE("io/utils/join/one_root",
          "join must properly join paths with only one absolute path")
{
    CHECK("/some/test/path" == join({"/some/test", "path"}));
    CHECK("/some/longer/test/path" == join({"/some/longer", "test", "path"}));
    CHECK("keeps/non/trailing/slashes" == join({"keeps", "non", "trailing", "slashes"}));
}

TEST_CASE("io/utils/join/multiple_roots",
          "join must properly join paths with multiple absolute paths")
{
    CHECK("/some/test/path" == join({"garbage/path", "/some", "test", "path"}));

}

TEST_CASE("io/utils/splitext/simple",
          "simple cases for split ext")
{
    std::pair<std::string, std::string> val = std::make_pair(
        "/root/path/test", "txt");
    CHECK(val == splitext("/root/path/test.txt"));

    val = std::make_pair("root/path/test", "txt");
    CHECK(val == splitext("root/path/test.txt"));
}

TEST_CASE("io/utils/splitext/dot_in_path",
          "splitext must work properly if a dot is in the path")
{
    std::pair<std::string, std::string> val = std::make_pair(
        "/root/path.git/test", "txt");
    CHECK(val == splitext("/root/path.git/test.txt"));

    val = std::make_pair("root/path.git/test", "txt");
    CHECK(val == splitext("root/path.git/test.txt"));
}
