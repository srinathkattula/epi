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
    typedef Request RequestT;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    server()
        : m_io_service()
    {}

    virtual ~server() {}

    /// Run the server's io_service loop.
    void run() {
        // The io_service::run() call will block until all asynchronous operations
        // have finished. While the server is running, there is always at least one
        // asynchronous operation outstanding: the asynchronous accept call waiting
        // for new incoming connections.
        m_io_service.run();
    }

    /// Stop the server.
    virtual void stop() {
        // Post a call to the stop function so that server::stop() is safe to call
        // from any thread.
        m_io_service.post(boost::bind(&server::handle_stop, this));
    }

protected:
    /// Handle a request to stop the server.
    virtual void handle_stop() {}
    
    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service         m_io_service;
    /// The handler for all incoming requests.
    request_handler<Request>        m_request_handler;

    typename RequestT::AllocatorT   m_allocator;
};


/// The TCP server.
template<class Request>
class tcp_server : public server< Request >
{
public:
    typedef server< Request > BaseT;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    tcp_server(const std::string& address, const std::string& port)
        : m_connection_manager()
        , m_acceptor(m_io_service)
        , m_new_connection(
            new tcp_connection<Request>(
                m_io_service, m_connection_manager, 
                m_request_handler, m_allocator))
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
            boost::bind(&tcp_server<Request>::handle_accept, this, boost::asio::placeholders::error));
    }

private:
    /// The connection manager which owns all live connections.
    connection_manager< tcp_connection<Request> >  m_connection_manager;
    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor  m_acceptor;
    /// The next connection to be accepted.
    typename connection_manager< tcp_connection<Request> >::connection_ptr m_new_connection;

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e) {
        if (!e) {
            m_connection_manager.start(m_new_connection);
            m_new_connection.reset(
                new tcp_connection<Request>(
                    m_io_service, m_connection_manager, 
                    m_request_handler, m_allocator));
            m_acceptor.async_accept(m_new_connection->socket(),
                boost::bind(&tcp_server<Request>::handle_accept, this, boost::asio::placeholders::error));
        }
    }

    /// Handle a request to stop the server.
    virtual void handle_stop() {
        // The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_service::run() call
        // will exit.
        m_acceptor.close();
        m_connection_manager.stop_all();
    }
};


/// The PIPE server
template<class Request>
class pipe_server : public server<Request>
{
public:
    typedef server<Request> BaseT;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    pipe_server(int fd_in, int fd_out)
        : m_pipe_connection(m_io_service, m_request_handler, fd_in, fd_out, m_allocator)
    {
        m_pipe_connection.start();
    }

    ~pipe_server() {
        m_pipe_connection.stop();
    }
private:
    pipe_connection<Request> m_pipe_connection;
};

} // namespace server
} // namespace ei

#endif // _SERVER_HPP
