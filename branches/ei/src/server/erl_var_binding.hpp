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

#ifndef __VARIABLEBINDING_HPP
#define __VARIABLEBINDING_HPP

#include <map>

#include "erl_term.hpp"

namespace ei {

class ErlTerm;

std::ostream& operator<< (std::ostream &out, VariableBinding &binding);

class VariableBinding {
    friend std::ostream& epi::type::operator<< (std::ostream &out, VariableBinding &binding);
protected:
    typedef std::map<std::string, boost::shared_ptr<ErlTerm> > ErlTermMap;

public:
    VariableBinding():mErlTermMap()  {}

	 /**
	  * Bind a value to a variable name. The binding will be updated
	  * id variableName is unbound. If its bound, it will ignore the new
      * value
	  * @param variableName
	  * @param term ErlTerm to bind. Should be != 0.
      */
	 void bind( const std::string& variableName, ErlTerm* term ) {
        // bind only if is unbound
        if (mErlTermMap.find(variableName) == mErlTermMap.end())
            mErlTermMap[variableName] = term;
     }

	 /**
	  * Search for a variable by name
	  * @param variableName variable to search
      * @return bound ErlTerm pointer if variable is bound, 0 otherwise
      */

	 ErlTerm* search( const std::string& variableName ) const {
         ErlTermMap::const_iterator p = mErlTermMap.find(variableName);

         return p == mErlTermMap.end() ? 0 : p->second.get();
    }

	/**
	 * Merge all data in a variable binding with data in this
	 * variable binding.
     * @param binding pointer to binding to use.
     */
    void merge( VariableBinding *binding ) {
        for (ErlTermMap::const_iterator it = binding->mErlTermMap.begin(), end=binding->mErlTermMap.end();
             it != end; ++it)
        {
            bind(it->first, it->second.get());
        }
    }

	/**
	 * Reset this binding 
	 */
	void reset() {
		mErlTermMap.clear();
	}
	
protected:
    ErlTermMap mErlTermMap;
};


} // ei


#endif
