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
#include "../putget.h"

namespace ei {
namespace server {

/// The common handler for all incoming requests.
template <class request>
class request_handler : private boost::noncopyable
{
public:
    /// Construct with a directory containing files to be served.
    request_handler()
        : m_len(0)
        , m_offset(0)
    {}

    /// Handle a request and produce a reply.
    void handle_request(const request& req);

    /// Parse some data. The bool return value is true when a complete request
    /// has been parsed, false when more data is required. The <data> and <len> 
    /// return value indicates how much of the input has been consumed.
    bool parse(request& req, char*& data, size_t& len);

    /// Reset to initial parser state.
    void reset() { m_len = m_offset = 0; }

private:
    size_t m_len, m_offset;
    char   m_temp_buf[8];
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

template <class request>
bool request_handler<request>::parse(request& req, char*& data, size_t& len)
{
    if (m_len == 0) {
        int sz = std::min(4 - m_offset, len);
        assert( sz >= 0 );
        
        memcpy(m_temp_buf + m_offset, data, sz);
        m_offset += sz;
        len      -= sz;
        data     += sz;

        if (m_offset == 4) {
            const char* s = m_temp_buf;
            m_len    = get32be(s);
            m_offset = 0;
            req.init(m_len, data, len);
        } else {
            return false;
        }
    } else {
        req.copy(data, len);
    }

    bool result = req.full();

    if (result)
        m_len = 0;
    
    return result;
}


} // namespace server
} // namespace ei

#endif // _REQUEST_HANDLER_HPP
