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
    return mString;
}
