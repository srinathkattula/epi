//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _REQUEST_HPP
#define _REQUEST_HPP

#include <string>
#include <vector>
#include <boost/shared_array.hpp>
#include "debug.hpp"

namespace ei {
namespace server {

/// A request received from a client.
template <class Allocator>
class request
{
private:
    typedef request<Allocator> SelfT;

    Allocator& m_allocator;
    size_t m_size;
    size_t m_offset;
    boost::shared_array<char> m_buffer;
    
    struct bufferDeleter
    {
        SelfT& m_owner;
        bufferDeleter(SelfT& owner) : m_owner(owner) {}
        
        void operator() (char* p) {
            DBG("Deallocating 0x" << std::hex << (int)p << std::dec);
            if (p)
                m_owner.m_allocator.destroy(p);
        }
    };
    
public:
    typedef Allocator AllocatorT;

    request(Allocator& alloc) 
        : m_allocator(alloc)
        , m_size(0)
        , m_offset(0)
    {
    }

    void init(size_t tot_size, char*& p, size_t& sz) {
        char* buf = m_allocator.allocate(tot_size, 0);
        boost::shared_array<char> tmp(buf, bufferDeleter(*this));
        m_buffer.swap(tmp);
        DBG("Allocated " << tot_size << " bytes: 0x" << std::hex << (int)buf <<
            " m_buff: 0x" << (int)m_buffer.get() << " use_count: " << std::dec << m_buffer.use_count());
        m_size   = tot_size;
        m_offset = 0;
        copy(p, sz);
    }
    
    // Copies up to size-offset bytes from p to this->buffer, and
    // adjusts the <p> and <sz> to exclude copied data.
    bool copy(char*& p, size_t& sz) {
        size_t need = std::min( space(), sz );
        bool   res  = need > 0;
        if (res) {
            memcpy(m_buffer.get(), p, need);
            m_offset += need;
            sz       -= need;
            p        += need;
        }
        return res;
    }
    
    size_t space()  const { return m_size - m_offset; }
    bool   full()   const { return m_size - m_offset == 0; }
    size_t size()   const { return m_size; }
    size_t offset() const { return m_offset; }
    boost::shared_array<char> buffer() const { return m_buffer; }

    void reset() {
        m_size = m_offset = 0;
        m_buffer.reset();
    }
};

} // namespace server
} // namespace ei

#endif // _REQUEST_HPP
