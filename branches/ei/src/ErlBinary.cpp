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
#include <string.h>

#include "ErlBinary.hpp"
#include "EpiBuffer.hpp"

using namespace epi::error;
using namespace epi::type;

ErlBinary::ErlBinary(const char* buf, int* index) 
    throw(EpiEIDecodeException)
{
    const char *str = buf + *index;
    const char *s0 = str;

    if (get8(str) != ERL_BINARY_EXT)
        throw EpiEIDecodeException("Error decoding binary", *index);

    mSize = get32be(str);
    mData = new char[mSize];
    if (!mData)
        throw EpiEIDecodeException("Error decoding binary - out of memory", mSize);
    ::memcpy(mData,str,mSize);

    *index += str + mSize - s0;
    mDelete = true;
    mInitialized = true;
}

void ErlBinary::init(const void *data, const int size,
                const bool copy, const bool del)
    throw(EpiAlreadyInitialized)
{

    if (isValid()) {
        throw EpiAlreadyInitialized("Binary is initilialized");
    }

    // Copy the data if necesary
    mSize=size;
    if (copy) {
        mData = new char[size];
        memcpy(mData, data, size);
    } else {
        mData = (char*) data;
    }

    mDelete = del;
    mInitialized = true;
}

bool ErlBinary::equals(const ErlTerm &t) const {
    if (!t.instanceOf(ERL_BINARY))
        return false;

    if (!this->isValid() || !t.isValid())
        return false;


    ErlBinary *_t = (ErlBinary*) &t;
    return this->mSize == _t->mSize && memcmp(this->mData, _t->mData, this->mSize)==0;
}

std::string ErlBinary::toString(const VariableBinding *binding) const {
    if (!isValid())
        return "** INVALID BINARY **";

    std::ostringstream oss;
    oss << "<<";
    for(int i=0; i<mSize; i++) {
        oss << (unsigned int)mData[i];
        if (i < mSize-1)
            oss << ',';
    }
    oss << ">>";
    return oss.str();
}
