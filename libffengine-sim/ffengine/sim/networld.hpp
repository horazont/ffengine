/**********************************************************************
File name: networld.hpp
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
#ifndef SCC_SIM_NETWORLD_H
#define SCC_SIM_NETWORLD_H

#include <sigc++/signal.h>

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/coded_stream.h>

#include <QTcpServer>
#include <QTcpSocket>

#include "ffengine/sim/world.hpp"

#include "netserver_control.pb.h"

namespace sim {


typedef uint64_t NetConnectionID;
typedef std::unique_ptr<google::protobuf::Message> AbstractMessagePtr;


enum NetMessageClass: uint32_t {
    /**
     * Control messages are messages such as ping. These are used by the
     * server/client implementation and are not exported over the
     * IServerClientInterface.
     *
     * They are special in the sense that they are not passed to the
     * IMessageHandler, but to the TCPServerClient for handling.
     */
    MSGCLASS_LINK_CONTROL,

    /**
     * A world command maps directly to the corresponding protobuf.
     */
    MSGCLASS_WORLD_COMMAND
};


/**
 * The IMessageHandler interface is used to dispatch the different
 * NetMessageClass messages to different methods.
 *
 * Each method handles one NetMessageClass, although not all NetMessageClass
 * values are handled in the IMessageHandler (MSGCLASS_LINK_CONTROL is handled
 * separately).
 *
 * The handler methods return true if handling succeeded and false otherwise.
 * If a handler method returns false, the client is disconnected for violating
 * the protocol.
 */
class IMessageHandler
{
protected:
    /**
     * Default message handler. This is called in all default implementations
     * of the specific handlers and must be implemented in subclasses.
     *
     * This is to allow future expansion.
     *
     * @param msg The unhandled message.
     * @return true if message processing shall continue and false if the
     * connection should terminate.
     */
    virtual bool msg_unhandled(AbstractMessagePtr &&msg) = 0;

public:
    /**
     * Handle a messages::WorldCommand message.
     *
     * @param cmd The command message.
     */
    virtual bool msg_world_command(std::unique_ptr<messages::WorldCommand> &&cmd);

};


/**
 * The RejectingMessageHandler class re-implements
 * IMessageHandler::msg_unhandled() to always return false.
 */
class RejectingMessageHandler: public IMessageHandler
{
protected:
    bool msg_unhandled(AbstractMessagePtr &&msg) override final;
};


/**
 * A NetMessageParser is used to read the shim protocol which is used to wrap
 * protobuf messages.
 *
 * As protobuf messages are not self-delimiting, applications are required to
 * create their own wrapping protocol which determines the size of a single
 * message.
 *
 * The basic usage is to request a buffer to write to using next_buffer() and
 * confirming that it has been written using written(). During the call to
 * written(), NetMessageParser evaluates the buffer contents (if all required
 * data has been received) and fires events according to what has been found.
 *
 * The \a link_control_cb function is called when a
 * NetMessageClass::MSGCLASS_LINK_CONTROL is received; otherwise, the
 * IMessageHandler set using set_message_handler() receives the parsed
 * messages.
 *
 * Unknown message codes are handled by calling \a error_cb and resetting the
 * NetMessageParser.
 */
class NetMessageParser
{
public:
    // this allows for transferring a 60×60 chunk of 16 floats +
    // a bit of overhead (1024 byte)
    static constexpr uint32_t MAX_MESSAGE_SIZE = 60*60*16*sizeof(float)+1024;

    // the header consists of a uint32_t which denotes the size of the payload
    // and a uint32_t which denotes the class.
    static constexpr int64_t HEADER_SIZE = sizeof(NetMessageClass)+sizeof(uint32_t);

    enum ReceptionState {
        RECV_WAIT_FOR_HEADER,
        RECV_PAYLOAD
    };

    using LinkControlCallback = std::function<void(std::unique_ptr<messages::NetWorldControl>&&)>;
    using ErrorCallback = std::function<void()>;

public:
    /**
     * Construct a new NetMessageParser, initialising the corresponding
     * callbacks.
     *
     * @see NetMessageParser
     *
     * @param link_control_cb The callback to call when a MSGCLASS_LINK_CONTROL
     * message arrives.
     * @param error_cb The callback to call when an error occurs.
     * @param id The connection ID; this is only used for logging.
     */
    NetMessageParser(LinkControlCallback &&link_control_cb,
                     ErrorCallback &&error_cb,
                     const NetConnectionID &id);

private:
    const NetConnectionID m_id;

    LinkControlCallback m_link_control_cb;
    ErrorCallback m_error_cb;
    IMessageHandler *m_message_handler;
    std::string m_recv_buffer;
    ReceptionState m_recv_state;
    uint64_t m_recv_barrier;
    uint64_t m_written_up_to;

    NetMessageClass m_curr_class;

private:
    /**
     * Call reset() and emit the error_cb.
     */
    void fail();

    /**
     * Parse the current buffer contents into the given message \a dest.
     *
     * @param dest A protobuf message to parse the buffer into.
     * @return true if parsing succeeded and false otherwise.
     */
    bool parse(::google::protobuf::Message &dest);

    /**
     * Process a message header in the buffer.
     */
    void received_header();

    /**
     * Process a message payload in the buffer.
     */
    void received_payload();

    /**
     * Check the current state and call the corresponding function to process
     * the buffer contents.
     */
    void received_to_barrier();

public:
    /**
     * Reset the NetMessageParser.
     *
     * This releases most memory which has been dynamically allocated for
     * parsing and resets the parser into a state suitable for starting to
     * parse.
     */
    void reset();

    /**
     * Request a pointer to write data to.
     *
     * @return A pointer and a size, forming a buffer the user can write to.
     *
     * The function returns a pointer and a size. The size is the maximum
     * number of bytes the user is allowed to write to the memory pointed to
     * by the pointer.
     *
     * After data has been written, written() must be called to commit it.
     */
    std::pair<char*, size_t> next_buffer();

    /**
     * Set a IMessageHandler which is used for handling the non-link-control
     * messages.
     *
     * The default handler and the handler which is used when nullptr is set as
     * handler is a RejectingMessageHandler.
     *
     * @param handler The handler to use; may be nullptr to use a
     * RejectingMessageHandler.
     */
    void set_message_handler(IMessageHandler *handler);

    /**
     * Commit \a bytes bytes written to the last buffer returned by
     * next_buffer().
     *
     * This must be called exactly once after each next_buffer() call to inform
     * the NetMessageParser that the user has finished writing data to the
     * returned buffer. The user must not write further data to the buffer
     * after the call to written().
     *
     * If more data arrives, the user must use next_buffer() to get a new
     * pointer-size-pair.
     *
     * @param bytes The number of bytes actually written to the buffer. This
     * must be less than or equal to the size returned by next_buffer() and
     * may be 0.
     */
    void written(size_t bytes);

};


class TCPServerClient: public QObject, public IServerClientInterface
{
public:
    explicit TCPServerClient(qintptr descriptor, QObject *parent = 0);
    TCPServerClient(const TCPServerClient &ref) = delete;
    TCPServerClient &operator=(const TCPServerClient &ref) = delete;
    TCPServerClient(TCPServerClient &&src) = delete;
    TCPServerClient &operator=(TCPServerClient &&src) = delete;
    ~TCPServerClient() override;

private:
    static std::atomic<uint64_t> m_connection_id_ctr;

    const uint64_t m_connection_id;
    bool m_terminated;
    QTcpSocket m_socket;
    std::string m_send_buffer;
    sigc::signal<void> m_sig_disconnected;
    NetMessageParser m_message_parser;

private:
    void fail();
    void link_control_received(std::unique_ptr<messages::NetWorldControl> &&msg);

protected slots:
    void on_data_received();
    void on_disconnected();
    void on_error(QAbstractSocket::SocketError err);

public:
    sigc::signal<void> &disconnected() override;
    void flush() override;
    void send_message(const google::protobuf::Message &msg) override;
    void terminate() override;

};


class TCPServer: public QTcpServer
{
public:
    explicit TCPServer(QObject *parent = 0);

private:
    std::vector<std::unique_ptr<TCPServerClient> > m_clients;

protected:
    void incomingConnection(qintptr descriptor) override;
    void closeAllClients();

};

}

#endif
