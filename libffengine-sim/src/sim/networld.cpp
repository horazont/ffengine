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
#define QT_NO_EMIT

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "ffengine/sim/networld.hpp"

#include "world_command.pb.h"

#include <iostream>


namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.networld");

static RejectingMessageHandler default_message_handler;

/* sim::IMessageHandler */

bool IMessageHandler::msg_world_command(std::unique_ptr<messages::WorldCommand> &&cmd)
{
    return msg_unhandled(std::move(cmd));
}

/* sim::RejectingMessageHandler */

bool RejectingMessageHandler::msg_unhandled(AbstractMessagePtr &&)
{
    return false;
}

/* sim::NetMessageParser */

NetMessageParser::NetMessageParser(LinkControlCallback &&link_control_cb,
                                   ErrorCallback &&error_cb,
                                   const NetConnectionID &id):
    m_id(id),
    m_link_control_cb(std::move(link_control_cb)),
    m_error_cb(std::move(error_cb)),
    m_message_handler(&default_message_handler),
    m_recv_state(RECV_WAIT_FOR_HEADER),
    m_recv_barrier(HEADER_SIZE),
    m_written_up_to(0)
{
    m_recv_buffer.resize(m_recv_barrier);
}

void NetMessageParser::fail()
{
    reset();
    m_error_cb();
}

bool NetMessageParser::parse(google::protobuf::Message &dest)
{
    return dest.ParseFromArray(m_recv_buffer.data(), m_recv_buffer.size());
}

void NetMessageParser::received_header()
{
    const uint8_t *buffer = reinterpret_cast<const uint8_t*>(m_recv_buffer.data());

    uint32_t msgclass = 0;
    uint32_t msgsize = 0;
    bool success = true;
    success = success && google::protobuf::io::CodedInputStream::ReadLittleEndian32FromArray(
                &buffer[0], &msgclass);
    success = success && google::protobuf::io::CodedInputStream::ReadLittleEndian32FromArray(
                &buffer[sizeof(msgclass)], &msgsize);

    if (!success) {
        logger.log(io::LOG_ERROR) << "connection " << m_id
                                  << " failed to receive header"
                                  << io::submit;
        fail();
        return;
    }

    if (msgsize > MAX_MESSAGE_SIZE) {
        logger.log(io::LOG_WARNING) << "connection " << m_id
                                    << " attempted to send too large message ("
                                    << msgsize << " bytes, max is "
                                    << MAX_MESSAGE_SIZE << "). "
                                    << "protocol violation, killing."
                                    << io::submit;
        fail();
        return;
    }

    m_recv_barrier = msgsize;
    m_recv_state = RECV_PAYLOAD;
    m_curr_class = NetMessageClass(msgclass);
}

void NetMessageParser::received_payload()
{
    bool pass = false;
    switch (m_curr_class)
    {
    case MSGCLASS_LINK_CONTROL:
    {
        auto protobuf = std::make_unique<messages::NetWorldControl>();
        if (!parse(*protobuf)) {
            fail();
            return;
        }
        m_link_control_cb(std::move(protobuf));
        pass = true;
        break;
    }
    case MSGCLASS_WORLD_COMMAND:
    {
        auto protobuf = std::make_unique<messages::WorldCommand>();
        if (!parse(*protobuf)) {
            fail();
            return;
        }
        pass = m_message_handler->msg_world_command(std::move(protobuf));
        break;
    }
    }

    if (!pass) {
        fail();
        return;
    }

    m_recv_state = RECV_WAIT_FOR_HEADER;
    m_recv_barrier = HEADER_SIZE;
}

void NetMessageParser::received_to_barrier()
{
    switch (m_recv_state)
    {
    case RECV_WAIT_FOR_HEADER:
    {
        received_header();
        break;
    }
    case RECV_PAYLOAD:
    {
        received_payload();
        break;
    }
    }
    m_written_up_to = 0;
    m_recv_buffer.resize(m_recv_barrier);
}

void NetMessageParser::reset()
{
    m_recv_buffer.clear();
    m_recv_buffer.shrink_to_fit();
    m_recv_state = RECV_WAIT_FOR_HEADER;
    m_recv_barrier = HEADER_SIZE;
    m_written_up_to = 0;
}

std::pair<char *, size_t> NetMessageParser::next_buffer()
{
    size_t to_recv = m_recv_barrier - m_written_up_to;
    return std::make_pair(&m_recv_buffer[m_written_up_to], to_recv);
}

void NetMessageParser::set_message_handler(IMessageHandler *handler)
{
    if (!handler) {
        m_message_handler = &default_message_handler;
    } else {
        m_message_handler = handler;
    }
}

void NetMessageParser::written(size_t bytes)
{
    m_written_up_to += bytes;
    if (m_written_up_to > m_recv_barrier) {
        throw std::logic_error("NetMessageParser user wrote more bytes than allowed");
    }

    if (m_written_up_to == m_recv_barrier) {
        received_to_barrier();
    }
}


/* sim::TCPServerClient */

std::atomic<uint64_t> TCPServerClient::m_connection_id_ctr(0);

TCPServerClient::TCPServerClient(qintptr descriptor, QObject *parent):
    QObject(parent),
    m_connection_id(m_connection_id_ctr.fetch_add(1)),
    m_terminated(false),
    m_socket(nullptr),
    m_message_parser(std::bind(&TCPServerClient::link_control_received,
                               this,
                               std::placeholders::_1),
                     std::bind(&TCPServerClient::fail,
                               this),
                     m_connection_id)
{
    logger.log(io::LOG_INFO) << "new connection with id" << m_connection_id << io::submit;

    m_socket.setSocketDescriptor(descriptor);
    connect(&m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, &TCPServerClient::on_error,
            Qt::DirectConnection);
    connect(&m_socket, &QTcpSocket::disconnected,
            this,&TCPServerClient::on_disconnected,
            Qt::DirectConnection);
    connect(&m_socket, &QTcpSocket::readyRead,
            this, &TCPServerClient::on_data_received,
            Qt::DirectConnection);
}

TCPServerClient::~TCPServerClient()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        if (m_socket.state() == QAbstractSocket::ConnectedState) {
            logger.log(io::LOG_WARNING) << "connection " << m_connection_id
                                        << " still open during destruction; "
                                        << " attempting graceful shutdown..."
                                        << io::submit;
            m_socket.disconnectFromHost();
        } else {
            logger.log(io::LOG_INFO) << "connection " << m_connection_id
                                     << " closing during destruction"
                                     << io::submit;
        }
        if (m_socket.waitForDisconnected()) {
            logger.log(io::LOG_INFO) << "connection " << m_connection_id
                                        << " closed"
                                        << io::submit;
        } else {
            logger.log(io::LOG_WARNING) << "connection " << m_connection_id
                                        << " failed to close in time"
                                        << io::submit;
        }
    }
    logger.log(io::LOG_INFO) << "connection " << m_connection_id
                             << " destroyed" << io::submit;
}

void TCPServerClient::fail()
{
    terminate();
    // emit the signal as it won’t be emitted due to terminate()ion
    m_sig_disconnected.emit();
}

void TCPServerClient::link_control_received(std::unique_ptr<messages::NetWorldControl> &&msg)
{

}

void TCPServerClient::on_data_received()
{
    char *dest;
    size_t size;
    while (m_socket.bytesAvailable() > 0) {
        std::tie(dest, size) = m_message_parser.next_buffer();
        if (size > 0) {
            int64_t bytes_read = m_socket.read(dest, size);
            m_message_parser.written(bytes_read);
        } else {
            m_message_parser.written(0);
        }
    }
}

void TCPServerClient::on_disconnected()
{
    logger.log(io::LOG_INFO) << "connection " << m_connection_id
                             << " disconnected" << io::submit;
    if (!m_terminated) {
        m_sig_disconnected.emit();
    }
}

void TCPServerClient::on_error(QAbstractSocket::SocketError err)
{
    logger.log(io::LOG_ERROR)
            << "connection " << m_connection_id
            << " failed with " << m_socket.errorString().toStdString()
            << " (Qt errno " << err << ")"
            << io::submit;
}

sigc::signal<void> &TCPServerClient::disconnected()
{
    return m_sig_disconnected;
}

void TCPServerClient::flush()
{
    if (m_terminated) {
        logger.log(io::LOG_WARNING)
                << "attempt to flush terminated connection " << m_connection_id
                << io::submit;
        return;
    }
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        logger.log(io::LOG_WARNING)
                << "attempt to flush closed connection " << m_connection_id
                << io::submit;
        return;
    }
    m_socket.flush();
}

void TCPServerClient::send_message(const google::protobuf::Message &msg)
{
    if (m_terminated) {
        logger.log(io::LOG_WARNING)
                << "attempt to send to terminated connection " << m_connection_id
                << io::submit;
        return;
    }
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        logger.log(io::LOG_WARNING)
                << "attempt to send to closed connection " << m_connection_id
                << io::submit;
        return;
    }

    m_send_buffer.clear();
    {
        google::protobuf::io::StringOutputStream outstream(&m_send_buffer);
        msg.SerializeToZeroCopyStream(&outstream);
    }
    int64_t written = m_socket.write(m_send_buffer.data(), m_send_buffer.size());
    if (written < 0) {
        logger.logf(io::LOG_WARNING, "failed to write data to QTcpSocket");
    }
}

void TCPServerClient::terminate()
{
    if (m_terminated) {
        return;
    }

    m_send_buffer.clear();
    m_send_buffer.shrink_to_fit();
    m_message_parser.reset();
    m_socket.close();
    m_terminated = true;
}

/* sim::TCPServer */

TCPServer::TCPServer(QObject *parent):
    QTcpServer(parent)
{

}

void TCPServer::incomingConnection(qintptr descriptor)
{
    auto client = std::make_unique<TCPServerClient>(descriptor, nullptr);
    m_clients.emplace_back(std::move(client));
}

void TCPServer::closeAllClients()
{
    m_clients.clear();
}

}
