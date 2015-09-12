/**********************************************************************
File name: server.hpp
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
#ifndef SSC_SIM_SERVER_H
#define SCC_SIM_SERVER_H

#include <sigc++/sigc++.h>

#include <memory>
#include <shared_mutex>

#include <google/protobuf/message.h>

#include <QObject>

#include "ffengine/sim/world.hpp"


namespace sim {

typedef std::unique_ptr<google::protobuf::Message> AbstractMessagePtr;

namespace messages {

class WorldCommand;

}

/**
 * The IMessageHandler interface is used to dispatch the different classes of
 * messages exchanged between client and server.
 *
 * The handler methods return true if handling succeeded and false otherwise.
 * If a handler method returns false, the client is disconnected for violating
 * the protocol.
 */
class IMessageHandler
{
public:
    virtual ~IMessageHandler();

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
 *
 * The RejectingMessageHandler is thread-safe.
 */
class RejectingMessageHandler: public IMessageHandler
{
protected:
    bool msg_unhandled(AbstractMessagePtr &&msg) override final;
};


/**
 * Base class used to implement objects serving as interfaces for a server to
 * talk to clients.
 *
 * The IMessageHandler implementation is thread-safe, but sending of messages
 * is only guaranteed to happen when flush() is called.
 */
class ServerClientBase: public QObject, public IMessageHandler
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~ServerClientBase() override;

signals:
    /**
     * This signal is emitted when the ServerClientBase is disconnected.
     *
     * After this signal has been emitted, no methods must be called on the
     * object anymore. Thus, it is only safe to use with a Qt::DirectConnection
     * and custom synchronization in the slot.
     */
    void disconnected();

public slots:
    /**
     * Request that any pending data to send is flushed.
     *
     * This slot is not thread-safe, but must be invoked using
     * Qt::QueuedConnection. It will take all messages enqueued using
     * send_message() and send them to the client. Afterwards, any buffers are
     * flushed.
     */
    virtual void flush() = 0;

    /**
     * Close the connection of a client.
     *
     * After terminate() has been called, no messages must be sent over the
     * client interface and the client interface will not emit messages anymore,
     * even if messages are received from the peer.
     *
     * However, as terminate() must be called using Qt::QueuedConnection,
     * it is safer to first unset the message handler.
     */
    virtual void terminate() = 0;

public:
    /**
     * Set the message handler which receives incoming messages.
     *
     * The methods of the handler are called from within any thread;
     * synchronization needs to happen manually.
     *
     * @param handler The handler to use.
     */
    virtual void set_message_handler(IMessageHandler *handler) = 0;

};


class Server
{
public:
    typedef std::shared_lock<std::shared_timed_mutex> SyncSafeLock;

public:
    Server();
    ~Server();

private:
    WorldState m_state;

    /* guarded by m_clients_mutex */
    std::mutex m_clients_mutex;
    std::vector<ServerClientBase*> m_client_interfaces;

    /* guarded by m_op_queue_mutex */
    std::mutex m_op_queue_mutex;
    std::vector<std::unique_ptr<WorldOperation> > m_op_queue;

    std::atomic_bool m_terminated;
    std::thread m_game_thread;

    /* owned by m_game_thread */
    std::vector<std::unique_ptr<WorldOperation> > m_op_buffer;

    /**
     * This mutex is used to put the Server into a state which is safe for
     * syncing, that is, reading arbitrary data.
     */
    std::shared_timed_mutex m_interframe_mutex;

protected:
    void game_frame();
    void game_thread();

public:
    inline WorldState &state()
    {
        return m_state;
    }

    inline const WorldState &state() const
    {
        return m_state;
    }

    /**
     * Thread-safely enqueue a world operation for the next game frame.
     *
     * There won’t be any feedback on the operation. This is generally not
     * useful for actual client-server usage, but for local terraforming it
     * is quite handy.
     *
     * @param op Operation to execute in the next game frame.
     */
    void enqueue_op(std::unique_ptr<WorldOperation> &&op);

    /**
     * Return a lock object on the WorldState and ensure that simulations are
     * in a state where their front buffers / data can be read safely.
     *
     * This makes sure that the game loop stops until all locks are released
     * and that all simulations are in a state where they are readable.
     *
     * @return An opaque, movable object; it cannot be passed between threads.
     * As long as your thread holds such an object, it is safe to access the
     * world state for reading.
     */
    SyncSafeLock sync_safe_point();

};


}

#endif
