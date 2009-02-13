//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _REQUEST_HANDLER_HPP
#define _REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>
#include "request_parser.hpp"

namespace ei {
namespace server {

/// The common handler for all incoming requests.
template <class request>
class request_handler : private boost::noncopyable
{
public:
    /// Construct with a directory containing files to be served.
    request_handler() {}

    /// Handle a request and produce a reply.
    void handle_request(const request& req);
};


template <class request>
void request_handler<request>::handle_request(const request& req)
{
    std::string request_path;
    boost::shared_array<char> buffer = req.buffer();
    std::cout << "New request of size: " << req.size() << " (offset: " << req.offset() << ")" << std::endl;
    std::cout << "  use_count: " << buffer.use_count() << std::endl;
    std::cout << "  Msg: " << buffer.get() << std::endl;
}

} // namespace server
} // namespace ei

#endif // _REQUEST_HANDLER_HPP
