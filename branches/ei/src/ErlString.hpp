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

#ifndef _ERLSTRING_HPP
#define _ERLSTRING_HPP

#include "ErlList.hpp"

namespace epi {
namespace type {

/**
 * Provides a representation of Erlang strings.
 **/
class ErlString: public ErlTerm {

public:

    /**
     * Create an unitialized (and invalid) string
     **/
    inline ErlString() {
        Dout(dc::erlang, "Created unitialized String at" << this);
    };

    /**
     * Create an string from the given string.
     * @param string the string to create the ErlString from.
     **/
    ErlString(const std::string &string) {
        try{
            init(string);
        } catch (EpiAlreadyInitialized&) {
        }
    }

    ErlString(const char* buf, int* index) throw(EpiEIDecodeException);

    /**
     * Init this string with the given string.
     *
     * @param string the string to init the ErlString from.
     * @returns apiresult Result of operation.
     * @throws EpiAlreadyInitialized if the string is already initialized
     */
    void init(const std::string &string)
            throw (EpiAlreadyInitialized);

    /**
     * Get the actual string contained in this term.
     * @throws EpiInvalidTerm if the term is invalid
     **/
    std::string stringValue() const
            throw(EpiInvalidTerm);

    bool equals(const ErlTerm &t) const;

    std::string toString(const VariableBinding *binding = 0) const;

    inline ~ErlString() {
        Dout(dc::erlang, "Destroing String \"" <<
                mString << "\" at " << this);
    }

 	 IMPL_TYPE_SUPPORT(ErlString, ERL_STRING);

private:
    ErlString (const ErlString&) {}

protected:
    std::string mString;

};

} //namespace type
} //namespace epi

#endif // _ERLSTRING_HPP
