//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _CONNECTION_MANAGER_HPP
#define _CONNECTION_MANAGER_HPP

#include <vector>
#include <set>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "request.hpp"
#include "request_handler.hpp"
#include "debug.hpp"

namespace ei {
namespace server {

template <class Connection>
class connection_manager;

namespace posix = boost::asio::posix;

//------------------------------------------------------------------------------
// connection class
//------------------------------------------------------------------------------

/// Represents a single connection from a client.
template <typename Handler>
class connection
    : private boost::noncopyable
{
public:
    typedef connection<Handler> ConnectionT;
    typedef Handler             HandlerT;

    virtual ~connection() {}
    
    /// Start the first asynchronous operation for the connection.
    virtual void start() = 0;

    /// Stop all asynchronous operations associated with the connection.
    virtual void stop() {}

protected:
    /// Construct a connection
    connection( request_handler<Handler>& handler)
        : m_request_handler(handler)
    {}

    /// Handle completion of a read operation.
    void process_chunk(std::size_t bytes_transferred) {
        char* data = m_buffer.data();

        while (bytes_transferred > 0) {
            if (m_request_handler.parse(data, bytes_transferred)) {
                m_request_handler.handle_request();
            }
        }
    }

    /// The handler used to process the incoming request.
    request_handler<Handler>& m_request_handler;
    /// Buffer for incoming data.
    boost::array<char, 8192>  m_buffer;
};

//------------------------------------------------------------------------------
// tcp_connection class
//------------------------------------------------------------------------------
template <typename Handler>
class tcp_connection
    : public connection<Handler>
    , public boost::enable_shared_from_this< tcp_connection<Handler> >
{
public:
    typedef connection<Handler> BaseT;
    
    tcp_connection(boost::asio::io_service& io_service,
        connection_manager< tcp_connection<Handler> >& manager, 
        request_handler<Handler>& handler)
    : connection<Handler>(handler)
    , m_connection_manager(manager)
    , m_socket(io_service)
    {}
    
    virtual ~tcp_connection() 
    {}
    
    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket() { return m_socket; }

    void start() {
        m_peer_endpoint = m_socket.remote_endpoint();
        if (m_peer_endpoint.port() != 0) {
            DBG("New connection detected: " 
                    << m_peer_endpoint.address() << ":" << m_peer_endpoint.port());
        }
        m_socket.async_read_some(boost::asio::buffer(m_buffer),
            boost::bind(&tcp_connection<Handler>::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void stop() { 
        if (m_peer_endpoint.port() != 0)
            DBG("Destroying connection to " << m_peer_endpoint.address() << ":" << m_peer_endpoint.port());
        m_socket.close(); 
    }

private:
    /// The manager for this connection.
    connection_manager< tcp_connection<Handler> >& m_connection_manager;
    /// Socket for the connection.
    boost::asio::ip::tcp::socket     m_socket;
    boost::asio::ip::tcp::endpoint   m_peer_endpoint;

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
                     std::size_t bytes_transferred)
    {
        //BaseT::handle_read(e, bytes_transferred);
        if (e && e != boost::asio::error::operation_aborted) {
            m_connection_manager.stop(shared_from_this());
            return;
        }

        if (bytes_transferred == 0) {
            // Connection closed
            return;
        } else {
            process_chunk(bytes_transferred);

            m_socket.async_read_some(
                boost::asio::buffer(m_buffer),
                boost::bind(&tcp_connection<Handler>::handle_read, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)
            );
        }
    }

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e) {
        if (!e) {
            // Initiate graceful connection closure.
            boost::system::error_code ignored_ec;
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }

        if (e != boost::asio::error::operation_aborted) 
            m_connection_manager.stop(shared_from_this());
    }
};

//------------------------------------------------------------------------------
// pipe_connection class
//------------------------------------------------------------------------------
template <typename Handler>
class pipe_connection : public connection<Handler>
{
public:
    typedef connection<Handler> BaseT;

    pipe_connection(boost::asio::io_service& io_service,
        request_handler<Handler>& handler,
        int in_fd, int out_fd)
    : connection<Handler>(handler)
    , m_input (io_service, ::dup(in_fd))
    , m_output(io_service, ::dup(out_fd))
    {}
    
    virtual ~pipe_connection() 
    {}
    
    int input_handle()  { return m_input;  }
    int output_handle() { return m_output; }

    void start() {
        DBG("New pipe connection established"); 
        m_input.async_read_some(boost::asio::buffer(m_buffer),
            boost::bind(&pipe_connection<Handler>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    void stop() { 
        DBG("Destroying pipe connection");
    }

private:
    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
                     std::size_t bytes_transferred)
    {
        //BaseT::handle_read(e, bytes_transferred);
        if (e && e != boost::asio::error::operation_aborted)
            return;

        if (bytes_transferred == 0) {
            // Connection closed
            return;
        } else {
            process_chunk(bytes_transferred);

            m_input.async_read_some(boost::asio::buffer(m_buffer),
                boost::bind(&pipe_connection<Handler>::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
    }

    posix::stream_descriptor m_input;
    posix::stream_descriptor m_output;
};

//------------------------------------------------------------------------------
// connection_manager class
//------------------------------------------------------------------------------

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
template <class Connection>
class connection_manager : private boost::noncopyable
{
public:
    typedef boost::shared_ptr< Connection > connection_ptr;

    /// Add the specified connection to the manager and start it.
    void start(connection_ptr c) {
        m_connections.insert(c);
        c->start();
    }

    /// Stop the specified connection.
    void stop(connection_ptr c) {
        m_connections.erase(c);
        c->stop();
    }

    /// Stop all connections.
    void stop_all() {
        std::for_each(m_connections.begin(), m_connections.end(),
            boost::bind(&Connection::stop, _1));
        m_connections.clear();
    }

private:
    /// The managed connections.
    std::set<connection_ptr> m_connections;
};


//------------------------------------------------------------------------------
// Inlined connection template implementation
//------------------------------------------------------------------------------

} // namespace server
} // namespace ei

#endif // _CONNECTION_MANAGER_HPP
