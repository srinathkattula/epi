//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _SERVER_HPP
#define _SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace ei {
namespace server {

/// The top-level class of the HTTP server.
template<class Request>
class server : private boost::noncopyable
{
public:
    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    server(const std::string& address, const std::string& port);

    /// Run the server's io_service loop.
    void run() {
        // The io_service::run() call will block until all asynchronous operations
        // have finished. While the server is running, there is always at least one
        // asynchronous operation outstanding: the asynchronous accept call waiting
        // for new incoming connections.
        m_io_service.run();
    }

    /// Stop the server.
    void stop() {
        // Post a call to the stop function so that server::stop() is safe to call
        // from any thread.
        m_io_service.post(boost::bind(&server::handle_stop, this));
    }

private:
    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e);
    /// Handle a request to stop the server.
    void handle_stop();
    
    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service         m_io_service;
    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor  m_acceptor;
    /// The connection manager which owns all live connections.
    connection_manager<Request>     m_connection_manager;
    /// The next connection to be accepted.
    typename connection_manager<Request>::connection_ptr m_new_connection;
    /// The handler for all incoming requests.
    request_handler<Request>        m_request_handler;

    typename Request::AllocatorT    m_allocator;
};

template <class Request>
server<Request>::server(const std::string& address, const std::string& port)
    : m_io_service()
    , m_acceptor(m_io_service)
    , m_connection_manager()
    , m_new_connection(
        new connection<Request>(m_io_service,
                                m_connection_manager, 
                                m_request_handler,
                                m_allocator))
{
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(m_io_service);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
    m_acceptor.async_accept(m_new_connection->socket(),
        boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
}

template <class Request>
void server<Request>::handle_accept(const boost::system::error_code& e)
{
    if (!e) {
        m_connection_manager.start(m_new_connection);
        m_new_connection.reset(
            new connection<Request>(
                m_io_service, m_connection_manager, 
                m_request_handler, m_allocator));
        m_acceptor.async_accept(m_new_connection->socket(),
            boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
    }
}

template <class Request>
void server<Request>::handle_stop()
{
    // The server is stopped by cancelling all outstanding asynchronous
    // operations. Once all operations have finished the io_service::run() call
    // will exit.
    m_acceptor.close();
    m_connection_manager.stop_all();
}


} // namespace server
} // namespace ei

#endif // _SERVER_HPP
