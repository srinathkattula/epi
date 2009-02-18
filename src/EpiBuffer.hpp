/*
***** BEGIN LICENSE BLOCK *****

This file is part of the EPI (Erlang Plus Interface) Library.

Copyright (C) 2005 Hector Rivas Gandara <keymon@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/

#ifndef _EPIBUFFER_H
#define _EPIBUFFER_H

#include "EpiError.hpp"
#include <ei.h>

// Forward declaration of erlang types
namespace epi {
namespace type {
    class ErlAtom;
    class ErlBinary;
    class ErlConsList;
    class ErlDouble;
    class ErlEmptyList;
    class ErlList;
    class ErlLong;
    class ErlPid;
    class ErlPort;
    class ErlRef;
    class ErlString;
    class ErlTerm;
    class ErlTuple;
    class VariableBinding;
} // namespace epi
} // namespace types


namespace epi {
namespace ei {

/**
 * Implementation of EI Buffer with allocator
 */
template <int N = 1024, class Allocator = std::allocator<char> >
class EIBufferAlloc: private ei_x_buff
{
    char        m_stat_buffer[N];
    Allocator&  m_alloc;
    bool        m_owns_data;
public:
    static const int s_min_size  = N;
    static const int s_min_alloc = 256;

    EIBufferAlloc(Allocator& alloc, char* data = NULL, size_t sz = 0)
        : m_alloc(alloc) 
        , m_owns_data(data == NULL)
    {
        buff   = data ? data : m_stat_buffer;
        index  = 0;
        buffsz = data ? sz : N;
    }

    ~EIBufferAlloc()    { reset(); }

    size_t idx()        { return index;  }
    int*   pidx()       { return &index; }
    size_t size() const { return buffsz; }
    const char* get()       const { return buff; }
    ei_x_buff* operator&()  const { return (ei_x_buff*)this; }

    void reset() {
        if (buff != m_stat_buffer) {
            if (m_owns_data)
                m_alloc.dealocate(buff);
            buff = m_stat_buffer;
            buffsz = s_min_size;
        }
        index = 0;
    }

    int realloc(size_t need) {
        size_t new_sz = index + need;
        if (new_sz <= (size_t)buffsz)
            return 0;
        size_t size = new_sz + s_min_alloc;
        char* new_buff = m_alloc.allocate(size);
        if (!new_buff)
            return -1;
        memcpy(new_buff, buff, buffsz);
        buff   = new_buff;
        buffsz = size;
    }
};

}
}


namespace epi {
namespace node {

using namespace epi::error;
using namespace epi::type;


/**
 * Abstract buffer.
 */
class Buffer {

public:
    virtual ~Buffer() {}

    /**
     * Reset the buffer.
     */
    virtual void reset() = 0;

     /**
      * Reset the internal index
      */
    virtual void resetIndex() = 0;

};

} // node
} // epi
#endif
