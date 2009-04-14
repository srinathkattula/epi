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

#include "debug.hpp" // Main config file

#include "erl_variable.hpp"
#include "erl_var_binding.hpp"

using namespace ei;

std::string ErlVariable::toString(const VariableBinding *binding) const {
    if (mName == "_")
        return mName;
    ErlTerm* term = binding ? binding->search(mName) : 0;
    return term ? term->toString() : mName;
}

ErlVariable* ErlVariable::searchUnbound(const VariableBinding* binding) {
    if (mName == "_")
        return 0;
    ErlTerm* term = binding ? binding->search(mName) : 0;
    return term ? NULL : this;
}

ErlTerm* ErlVariable::subst(const VariableBinding* binding)
    throw (EpiInvalidTerm, EpiVariableUnbound)
{
    if (mName == "_")
        throw EpiVariableUnbound("_");
    ErlTerm* term = binding? binding->search(mName): 0;
    if (term) {
        return term;
    } else {
        throw EpiVariableUnbound(mName);
    }
}


bool ErlVariable::internalMatch(VariableBinding* binding, ErlTerm* pattern)
        throw(EpiVariableUnbound)
{
    Dout(trace::erlang, "Matching variable '"<< mName << "' with " << pattern->toString());
    // If is anonymous, return true :)
    if (mName == "_")
        return true;

    // first check if this variable is bound
    ErlTerm* boundValue = binding? binding->search(mName): 0;
    if (boundValue) {
        Dout(trace::erlang, "Variable is bound to '"<< boundValue->toString(binding) <<"'");
        // perforn the matching with the value
        return boundValue->internalMatch(binding, pattern);
    } else {
        // bind the variable
        if (binding != 0)  {
            Dout(trace::erlang, "Variable is unbound, binding. ");
            binding->bind(mName, pattern->subst(binding));
            Dout(trace::erlang, "'" << mName << "' bound to '" << pattern->toString(binding) << "'");
        }
        return true;
    }
}

