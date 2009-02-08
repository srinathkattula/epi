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

#ifndef _ERLATOM_HPP
#define _ERLATOM_HPP

#include "ErlTerm.hpp"

namespace epi {
namespace type {

/**
 * Provides a representation of Erlang atoms. Atoms can be
 * created from strings whose length is not more than
 * {@link #MAX_ATOM_LENGTH MAX_ATOM_LENGTH} characters.
 **/
class ErlAtom: public ErlTerm {
public:


    /**
     * Create an unitialized (and invalid) atom
     */
    ErlAtom(): ErlTerm() {}

    /**
     * Create an atom from the given string.
     * @param atom the string to create the atom from.
     * @throws EpiBadArgument if string size is greater than
     *   MAX_ATOM_LENGTH or empty
     **/
    inline ErlAtom(const std::string &atom)
            throw(EpiBadArgument): ErlTerm()
    {
        try {
            this->init(atom);
        } catch (EpiAlreadyInitialized&) {
        }
    }

    ErlAtom(const char* buf, int* index) throw(EpiEIDecodeException);

    /**
     * Init this atom with the given string.
     * @param atom the string to init the atom from.
     * @returns apiresult Result of operation.
     * @throws EpiBadArgument if string size is
     *   greater than MAX_ATOM_LENGTH. Results in no change
     *   in the atom (same value, same state).
     * @throws EpiAlreadyInitialized if the atom
     *   is already initialized
     */
    void init(const std::string &atom)
            throw(EpiBadArgument,
                  EpiAlreadyInitialized);

    /**
     * Get the actual string contained in this term.
     * @throws EpiInvalidTerm if the term is invalid
     */
    std::string atomValue() const throw(EpiInvalidTerm);

    bool equals(const ErlTerm &t) const;

    std::string toString(const VariableBinding *binding = 0) const;

    IMPL_TYPE_SUPPORT(ErlAtom, ERL_ATOM);

private:
    // Private constructor
    ErlAtom(const ErlAtom&) {}

protected:
    std::string mAtom;

};


} //namespace type
} //namespace epi

#endif // _ERLATOM_HPP
