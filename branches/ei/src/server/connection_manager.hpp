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
#include "request_parser.hpp"
#include "debug.hpp"

namespace ei {
namespace server {

template <class Request>
class connection_manager;

//------------------------------------------------------------------------------
// connection class
//------------------------------------------------------------------------------

/// Represents a single connection from a client.
template <typename Request>
class connection
    : public boost::enable_shared_from_this< connection<Request> >
    , private boost::noncopyable
{
public:
    /// Construct a connection with the given io_service.
    explicit connection(boost::asio::io_service& io_service,
                        connection_manager<Request>& manager, 
                        request_handler<Request>& handler,
                        typename Request::AllocatorT& allocator);

    ~connection() {
        if (m_peer_endpoint.port() != 0)
            DBG("Destroying connection to " << m_peer_endpoint.address() << ":" << m_peer_endpoint.port());
    }
    
    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket() { return m_socket; }

    /// Start the first asynchronous operation for the connection.
    void start();

    /// Stop all asynchronous operations associated with the connection.
    void stop() { m_socket.close(); }

private:
    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
                     std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e);

    /// Socket for the connection.
    boost::asio::ip::tcp::socket    m_socket;
    /// The manager for this connection.
    connection_manager<Request>&    m_connection_manager;
    /// The handler used to process the incoming request.
    request_handler<Request>&       m_request_handler;
    /// Buffer for incoming data.
    boost::array<char, 8192>        m_buffer;
    /// The incoming request.
    Request                         m_request;
    /// The parser for the incoming request.
    typename request_handler<Request>::parser m_request_parser;

    boost::asio::ip::tcp::endpoint m_peer_endpoint;
};

//------------------------------------------------------------------------------
// connection_manager class
//------------------------------------------------------------------------------

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
template <class Request>
class connection_manager : private boost::noncopyable
{
public:
    typedef boost::shared_ptr< connection< Request > > connection_ptr;

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
            boost::bind(&connection<Request>::stop, _1));
        m_connections.clear();
    }

private:
    /// The managed connections.
    std::set<connection_ptr> m_connections;
};


//------------------------------------------------------------------------------
// Inlined connection template implementation
//------------------------------------------------------------------------------

template <class T> 
connection<T>::connection(boost::asio::io_service& io_service,
        connection_manager<T>& manager, request_handler<T>& handler,
        typename T::AllocatorT& allocator)
    : m_socket(io_service)
    , m_connection_manager(manager)
    , m_request_handler(handler)
    , m_request(allocator)
    //, m_peer_port(0)
{
}

template <class T> 
void connection<T>::start()
{
    m_peer_endpoint = m_socket.remote_endpoint();
    if (m_peer_endpoint.port() != 0) {
        DBG("New connection detected: " 
            << m_peer_endpoint.address() << ":" << m_peer_endpoint.port());
    }

    m_socket.async_read_some(boost::asio::buffer(m_buffer),
        boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

template <class T> 
void connection<T>::handle_read(
    const boost::system::error_code& e, std::size_t bytes_transferred)
{
    if (e && e != boost::asio::error::operation_aborted) {
        m_connection_manager.stop(shared_from_this());
        return;
    }

    char* data = m_buffer.data();

    while (bytes_transferred > 0) {
        if (m_request_parser.parse(m_request, data, bytes_transferred)) {
            m_request_handler.handle_request(m_request);
        }
    }
            
    m_socket.async_read_some(
        boost::asio::buffer(m_buffer),
        boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)
    );
}

template <class T> 
void connection<T>::handle_write(const boost::system::error_code& e)
{
    if (!e) {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }

    if (e != boost::asio::error::operation_aborted) {
        m_connection_manager.stop(shared_from_this());
    }
}

} // namespace server
} // namespace ei

#endif // _CONNECTION_MANAGER_HPP
