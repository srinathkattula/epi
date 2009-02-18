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

#include "Config.hpp" // Main config file

#include <iostream>
#include <sstream>
#include <memory>

#include "ErlString.hpp"
#include "EpiBuffer.hpp"

using namespace epi::error;
using namespace epi::type;

ErlString::ErlString(const char* buf, int* index) 
    throw(EpiEIDecodeException)
{
    const char *s = buf + *index;
    const char *s0 = s;
    int etype = get8(s);

    switch (etype) {
        case ERL_STRING_EXT: {
            int len = get16be(s);
            mString.assign(s, len);
            s += len;
            break;
        }
        case ERL_LIST_EXT: {
            /* Really long strings are represented as lists of small integers.
             * We don't know in advance if the whole list is small integers,
             * but we decode as much as we can, exiting early if we run into a
             * non-character in the list.
             */
            int len = get32be(s);
            mString.reserve(len);
            for (int i=0; i<len; i++) {
                if ((etype = get8(s)) != ERL_SMALL_INTEGER_EXT)
                    throw EpiEIDecodeException("Error decoding string", s+i-s0);
                mString.at(i) = get8(s);
            }
            break;
        }
        case ERL_NIL_EXT:
            mString.erase();
            break;

        default:
            throw EpiEIDecodeException("Error decoding string type", etype);
    }

    *index += s-s0;
    mInitialized = true;
}

std::string ErlString::stringValue() const
        throw(EpiInvalidTerm)
{
    if (!isValid()) {
        throw EpiInvalidTerm("String is not initialized");
    }
    return mString;
}

void ErlString::init(const std::string &string)
        throw(EpiAlreadyInitialized)
{
    if (isValid()) {
        throw EpiAlreadyInitialized("String is initilialized");
    }

    mString = string;
    mInitialized = true;
}

bool ErlString::equals(const ErlTerm &t) const {
    if (!t.instanceOf(ERL_STRING))
        return false;

    if (!this->isValid() || !t.isValid())
        return false;

    ErlString *_t = (ErlString*) &t;
    return mString == _t->mString;
}

/* FIXME: return escaped string */
std::string ErlString::toString(const VariableBinding *binding) const {
    if (!isValid())
        return "** INVALID STRING **";
    std::stringstream s;
    s << '"' << mString << '"';
    return s.str();
}
