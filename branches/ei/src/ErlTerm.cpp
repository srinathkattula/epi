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

#include "ErlTypes.hpp"
#include "ErlTerm.hpp"
#include "ErlTermPtr.hpp"
#include "VariableBinding.hpp"

using namespace epi::error;
using namespace epi::type;

/*
std::string StringOfTermType(const TermType t) {
    std::string erl_type_names[] =
    { "ErlAtom", "ErlLong", "ErlDouble", "ErlString", "ErlRef",
    "ErlPort", "ErlPid", "ErlBinary", "ErlTuple", "ErlList",
    "ErlEmptyList", "ErlConsList" };
    return erl_type_names[t];
}
*/

// By default, return himself
ErlTerm* ErlTerm::subst(const VariableBinding* binding)
        throw (EpiInvalidTerm, EpiVariableUnbound)
{
    if (!isValid())
        throw EpiInvalidTerm("Element is not initialized");

    return this;
}

bool ErlTerm::match(ErlTerm* pattern, VariableBinding* binding)
        throw (EpiVariableUnbound)
{
    // Protect the given binding. Just change it if the match success
    std::auto_ptr<VariableBinding> dirtyBinding(
        binding? new VariableBinding(*binding):
                 new VariableBinding());
    bool success = internalMatch(dirtyBinding.get(), pattern);
    if (success && binding) {
        binding->merge(dirtyBinding.get());
    }
    return success;
}

bool ErlTerm::internalMatch(VariableBinding* binding, ErlTerm* pattern)
        throw (EpiVariableUnbound)
{
    Dout(dc::erlang, "ErlTerm::internalMatch()");
    // default behaviour.
    if (!this->isValid())
        return false;
    else if (pattern->instanceOf(ERL_VARIABLE)) {
        Dout(dc::erlang, "Pattern parameter is a variable, conmute");
        return pattern->internalMatch(binding, this);
    } else
        return equals(*pattern);
}


ErlTerm* ErlTerm::binary_to_term(const char* buf, int& idx, size_t tot_size) 
    throw(EpiDecodeException)
{
    Dout_continue(dc::buffer, _continue, " failed.", "binary_to_term: ");

    // Check if is there are more elements to decode
    if ((size_t)idx == tot_size)
        return 0;

    // check the type of next term:
    int type, size;

    int ei_res = ei_get_type(buf, &idx, &type, &size);
    if(ei_res < 0)
        throw EpiEIDecodeException("ei_get_type failed", ei_res);

    ErlTerm* returnTerm = 0;

    switch (type) {
    case ERL_ATOM_EXT:
        Dout_continued( "decoding an Atom: ");
        returnTerm = new ErlAtom(buf, &idx);
        break;

    case ERL_LARGE_TUPLE_EXT:
    case ERL_SMALL_TUPLE_EXT: {
        Dout_continued( "decoding a Tuple: ");
        int arity;

        ei_res = ei_decode_tuple_header(buf, &idx, &arity);
        if (ei_res < 0) {
            throw EpiEIDecodeException("EI tuple decoding failed", ei_res);
        }
        try {
            ErlTermPtr<ErlTuple> new_tuple(new ErlTuple(arity));
            for (int i=0; i < arity; i++)
                new_tuple->initElement(ErlTerm::binary_to_term(buf, idx, tot_size));
            returnTerm = new_tuple.drop();
        } catch(EpiEIDecodeException &e) {
            throw e;
        } catch(EpiDecodeException &e) {
            throw e;
        } catch(EpiException &e) {
            throw EpiDecodeException(e.getMessage());
        }
        break;
    }

    case ERL_STRING_EXT:
        Dout_continued( "decoding a String: ");
        returnTerm = new ErlString(buf, &idx);
        break;

    case ERL_LIST_EXT: {
        Dout_continued( "decoding a List: ");
        int arity;

        ei_res = ei_decode_list_header(buf, &idx, &arity);

        if (ei_res < 0)
            throw EpiEIDecodeException("EI list decoding failed", ei_res);
        else if (arity == 0) {
            // EI docs says that this not happens... but, let's add it
            returnTerm = new ErlEmptyList();
        } else {
            ErlTermPtr<ErlConsList> new_list(new ErlConsList(arity)) ;
            try {
                for(int i = 0; i < arity; i++)
                    new_list->addElement(ErlTerm::binary_to_term(buf, idx, tot_size));
                new_list->close(ErlTerm::binary_to_term(buf, idx, tot_size));
            } catch (EpiEIDecodeException &e) {
                throw e;
            } catch (EpiDecodeException &e) {
                throw e;
            } catch (EpiException &e) {
                throw EpiDecodeException(e.getMessage());
            }

            returnTerm = new_list.drop();
        }
        break;
    }

    case ERL_NIL_EXT: {
        Dout_continued( "decoding a Empty list: ");
        int arity;
        ei_res = ei_decode_list_header(buf, &idx, &arity);

        if(ei_res < 0 || arity != 0)
            throw EpiEIDecodeException("EI empty list decoding failed", ei_res);

        returnTerm = new ErlEmptyList();
        break;
    }

    case ERL_SMALL_INTEGER_EXT:
    case ERL_SMALL_BIG_EXT:
    case ERL_LARGE_BIG_EXT:
    case ERL_INTEGER_EXT: {
        Dout_continued( "decoding a Long: ");
        long long vlong;
        if ((ei_res = ei_decode_longlong(buf, &idx, &vlong)) < 0)
            throw EpiEIDecodeException("EI long decoding failed", ei_res);
        returnTerm = new ErlLong(vlong);
        break;
    }

    case NEW_FLOAT_EXT:
    case ERL_FLOAT_EXT: {
        Dout_continued( "decoding a Double: ");
        double vdouble;
        if ((ei_res = ei_decode_double(buf, &idx, &vdouble)) < 0)
            throw EpiEIDecodeException("EI double decoding failed", ei_res);
        returnTerm = new ErlDouble(vdouble);
        break;
    }

    case ERL_BINARY_EXT:
        Dout_continued( "decoding a Binary: ");
        returnTerm = new ErlBinary(buf, &idx);
        break;

    case ERL_PID_EXT: 
        Dout_continued("decoding a Pid: ");
        returnTerm = new ErlPid(buf, &idx);
        break;

    case ERL_REFERENCE_EXT:
    case ERL_NEW_REFERENCE_EXT:
        Dout_continued( "decoding a Ref: ");
        returnTerm = new ErlRef(buf, &idx);
        break;

    case ERL_PORT_EXT:
        Dout_continued( "decoding a Port: ");
        returnTerm = new ErlPort(buf, &idx);
        break;

    default:
        std::ostringstream oss;
        oss << "Unknown message content type " << type;

        Dout_continued( "Unknown message content type " << type);
        throw EpiEIDecodeException(oss.str());
        break;
    }

    Dout_finish(_continue, returnTerm->toString() << ".");
    return returnTerm;
}

std::ostream& std::operator<< (std::ostream& os, const ErlTerm* t)
{
    return os << t->toString() << std::endl;
}

std::ostream& std::operator<< (std::ostream& os, const ErlTerm& t)
{
    return os << t.toString() << std::endl;
}
