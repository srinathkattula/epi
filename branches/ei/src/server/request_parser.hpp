//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _REQUEST_PARSER_HPP_
#define _REQUEST_PARSER_HPP_

#include <algorithm>
#include <boost/tuple/tuple.hpp>
#include <assert.h>
#include "../putget.h"
#include "request.hpp"

namespace ei {
namespace server {

/// Parser for incoming requests.
template <class request>
class request_parser
{
public:
    /// Construct ready to parse the request method.
    request_parser()
        : m_len(0)
        , m_offset(0)
    {}
    
    /// Reset to initial parser state.
    void reset() { m_len = m_offset = 0; }

    /// Parse some data. The bool return value is true when a complete request
    /// has been parsed, false when more data is required. The <data> and <len> 
    /// return value indicates how much of the input has been consumed.
    bool parse(request& req, char*& data, size_t& len);

private:
    size_t m_len, m_offset;
    char   m_temp_buf[8];
};


template <class request>
bool request_parser<request>::parse(request& req, char*& data, size_t& len)
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

#endif // _REQUEST_PARSER_HPP_
