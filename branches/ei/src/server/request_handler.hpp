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
#include "request.hpp"

namespace ei {
namespace server {

/// The common handler for all incoming requests.
template <class handler>
class request_handler : private boost::noncopyable
{
public:
    typedef typename handler::AllocatorT AllocatorT;
    
    /// Construct with a directory containing files to be served.
    request_handler(handler& h)
        : m_handler(h)
        , m_request(h.allocator())
        , m_len(0)
        , m_offset(0)
    {}

    /// Handle a request by calling handler.handle_message(msg, size)
    void handle_request();

    /// Parse some data. The bool return value is true when a complete request
    /// has been parsed, false when more data is required. The <data> and <len> 
    /// return value indicates how much of the input has been consumed.
    bool parse(char*& data, size_t& len);

    /// Reset to initial parser state.
    void reset() { m_len = m_offset = 0; }

    request<AllocatorT>& request() { return m_request; }
private:
    handler& m_handler;
    ei::server::request<AllocatorT> m_request;
    size_t m_len, m_offset;
    char   m_temp_buf[8];
};


template <class T>
void request_handler<T>::handle_request()
{
    boost::shared_array<char> buffer = m_request.buffer();
    m_handler.handle_message(buffer.get(), m_request.size());
}

template <class T>
bool request_handler<T>::parse(char*& data, size_t& len)
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
            m_request.init(m_len, data, len);
        } else {
            return false;
        }
    } else {
        m_request.copy(data, len);
    }

    bool result = m_request.full();

    if (result)
        m_len = 0;
    
    return result;
}


} // namespace server
} // namespace ei

#endif // _REQUEST_HANDLER_HPP
