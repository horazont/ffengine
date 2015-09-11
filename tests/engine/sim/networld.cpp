/**********************************************************************
File name: networld.cpp
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

#include "ffengine/sim/networld.hpp"

#include "world_command.pb.h"

#include <iostream>

using namespace sim;


struct NetMessageParserTest: public IMessageHandler
{
    NetMessageParserTest():
        m_parser(std::bind(&NetMessageParserTest::on_link_control,
                           this,
                           std::placeholders::_1),
                 std::bind(&NetMessageParserTest::on_error,
                           this),
                 0),
        m_had_error(false),
        m_pass_unhandled(true)
    {
        m_parser.set_message_handler(this);
    }

    NetMessageParser m_parser;
    bool m_had_error;
    bool m_pass_unhandled;
    std::vector<sim::AbstractMessagePtr> m_found;

private:
    void on_link_control(std::unique_ptr<sim::messages::NetWorldControl> &&msg)
    {
        m_found.emplace_back(std::move(msg));
    }

    void on_error()
    {
        m_had_error = true;
    }

protected:
    bool msg_unhandled(sim::AbstractMessagePtr &&msg) override final
    {
        m_found.emplace_back(std::move(msg));
        return m_pass_unhandled;
    }

};

const auto HEADER_SIZE = NetMessageParser::HEADER_SIZE;


TEST_CASE("sim/networld/NetMessageParser/barriers_and_emission")
{
    char *dest;
    size_t size;

    std::string src;

    {
        messages::NetWorldControl msg;
        messages::NetWorldPing ping;
        ping.set_token(0x1234);
        ping.set_payload(0x5678);
        *msg.mutable_ping() = std::move(ping);

        msg.SerializeToString(&src);
    }

    NetMessageParserTest test;
    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                MSGCLASS_LINK_CONTROL,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                src.size(),
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == src.size());
    REQUIRE(size >= src.size());

    memcpy(dest, src.data(), src.size());
    test.m_parser.written(src.size());

    CHECK(test.m_found.size() == 1);
    REQUIRE(test.m_found.size() >= 1);
    CHECK_FALSE(test.m_had_error);

    google::protobuf::Message *abstract_msg = test.m_found[0].get();
    REQUIRE(abstract_msg);
    messages::NetWorldControl *msg = dynamic_cast<messages::NetWorldControl*>(abstract_msg);
    REQUIRE(msg);
    REQUIRE(msg->has_ping());
    REQUIRE_FALSE(msg->has_pong());

    CHECK(msg->ping().token() == 0x1234);
    CHECK(msg->ping().payload() == 0x5678);
}

TEST_CASE("sim/networld/NetMessageParser/enforce_maximum_payload_size")
{
    char *dest;
    size_t size;

    NetMessageParserTest test;
    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                42,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                NetMessageParser::MAX_MESSAGE_SIZE+1,
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    CHECK(test.m_had_error);
}

TEST_CASE("sim/networld/NetMessageParser/allow_maximum_payload_size")
{
    char *dest;
    size_t size;

    NetMessageParserTest test;
    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                42,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                NetMessageParser::MAX_MESSAGE_SIZE,
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    CHECK_FALSE(test.m_had_error);
}

TEST_CASE("sim/networld/NetMessageParser/fail_at_unknown")
{
    char *dest;
    size_t size;

    std::string src;

    {
        messages::WorldCommand msg;
        msg.SerializeToString(&src);
    }

    NetMessageParserTest test;
    test.m_pass_unhandled = false;

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                UINT32_MAX,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                src.size(),
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == src.size());
    REQUIRE(size >= src.size());

    memcpy(dest, src.data(), src.size());
    test.m_parser.written(src.size());

    CHECK(test.m_had_error);
}

TEST_CASE("sim/networld/NetMessageParser/fail_at_false_result_from_handler")
{
    char *dest;
    size_t size;

    std::string src;

    {
        messages::WorldCommand msg;
        msg.SerializeToString(&src);
    }

    NetMessageParserTest test;
    test.m_pass_unhandled = false;

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                MSGCLASS_WORLD_COMMAND,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                src.size(),
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == src.size());
    REQUIRE(size >= src.size());

    memcpy(dest, src.data(), src.size());
    test.m_parser.written(src.size());

    CHECK(test.m_found.size() == 1);
    REQUIRE(test.m_found.size() >= 1);

    CHECK(test.m_had_error);
}

TEST_CASE("sim/networld/NetMessageParser/pass_at_true_result_from_handler")
{
    char *dest;
    size_t size;

    std::string src;

    {
        messages::WorldCommand msg;
        msg.SerializeToString(&src);
    }

    NetMessageParserTest test;
    test.m_pass_unhandled = true;

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                MSGCLASS_WORLD_COMMAND,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                src.size(),
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == src.size());
    REQUIRE(size >= src.size());

    memcpy(dest, src.data(), src.size());
    test.m_parser.written(src.size());

    CHECK(test.m_found.size() == 1);
    CHECK_FALSE(test.m_had_error);
}

TEST_CASE("sim/networld/NetMessageParser/set_message_handler(nullptr)")
{
    char *dest;
    size_t size;

    std::string src;

    {
        messages::WorldCommand msg;
        msg.SerializeToString(&src);
    }

    NetMessageParserTest test;
    test.m_pass_unhandled = false;

    test.m_parser.set_message_handler(nullptr);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == HEADER_SIZE);
    REQUIRE(size >= HEADER_SIZE);

    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                MSGCLASS_WORLD_COMMAND,
                reinterpret_cast<uint8_t*>(&dest[0]));
    google::protobuf::io::CodedOutputStream::WriteLittleEndian32ToArray(
                src.size(),
                reinterpret_cast<uint8_t*>(&dest[4]));
    test.m_parser.written(8);

    std::tie(dest, size) = test.m_parser.next_buffer();
    CHECK(size == src.size());
    REQUIRE(size >= src.size());

    memcpy(dest, src.data(), src.size());
    test.m_parser.written(src.size());

    CHECK(test.m_found.size() == 0);
    CHECK(test.m_had_error);
}
