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

#include "ErlTuple.hpp"
#include "EpiBuffer.hpp"

using namespace epi::error;
using namespace epi::type;

ErlTuple::ErlTuple(int arity):
        ErlTerm(), mArity(arity), mArityDefined(false), mElementVector(0)
{

    try {
        init(arity);
    } catch (EpiAlreadyInitialized &) {
    }
}

ErlTuple::ErlTuple(ErlTerm *elems[], unsigned int arity)
        throw(EpiBadArgument):
        ErlTerm(), mArity(0), mArityDefined(false), mElementVector(0)
{
    try {
        init(elems, arity);
    } catch (EpiAlreadyInitialized &) {
    }
}

ErlTuple::ErlTuple(ErlTerm *elem)
        throw(EpiBadArgument):
        ErlTerm(), mArity(1), mArityDefined(false), mElementVector(0)
{
    ErlTerm *elems[1] = {elem};
    init(elems, 1);
}

ErlTuple::ErlTuple(ErlTerm *elem1, ErlTerm *elem2)
        throw(EpiBadArgument):
        ErlTerm(), mArity(2), mArityDefined(false), mElementVector(0)
{
    ErlTerm *elems[2] = {elem1, elem2};
    init(elems, 2);
}

ErlTuple::ErlTuple(ErlTerm *elem1, ErlTerm *elem2, ErlTerm *elem3)
        throw(EpiBadArgument):
        ErlTerm(), mArity(3), mArityDefined(false), mElementVector(0)
{
    ErlTerm *elems[3] = {elem1, elem2, elem3};
    init(elems, 3);
}

ErlTuple::ErlTuple(ErlTerm *elem1, ErlTerm *elem2,
                   ErlTerm *elem3, ErlTerm *elem4)
        throw(EpiBadArgument):
        ErlTerm(), mArity(3), mArityDefined(false), mElementVector(0)
{
    ErlTerm *elems[4] = {elem1, elem2, elem3, elem4};
    init(elems, 4);
}

void ErlTuple::init(unsigned int arity)
        throw(EpiAlreadyInitialized) {

    if (isValid() || mArityDefined) {
        throw EpiAlreadyInitialized("Tuple already initialized");
    }

    mArity = arity;
    mArityDefined = true;
    if (arity == 0) {
        mInitialized = true;
    } else {
        if (mElementVector.capacity() != arity)
            mElementVector.reserve(arity);
    }

}

void ErlTuple::init(ErlTerm *elems[], unsigned int arity)
        throw(EpiBadArgument, EpiAlreadyInitialized)
{
    init(arity);

    try {
        for(size_t i=0; i<arity; i++) {
            initElement(elems[i]);
        }
    }  catch (EpiInvalidTerm &) {
    }
}

ErlTuple* ErlTuple::initElement(ErlTerm *elem)
        throw(EpiInvalidTerm, EpiBadArgument, EpiAlreadyInitialized)
{
    Dout(dc::erlang, "["<<this<<"]" << "ErlTuple:initElement(" <<
            (elem? elem->toString() : std::string("null")) << ")");
    if (isValid()) {
        throw EpiAlreadyInitialized("All elements are initialized");
    }
    if (!mArityDefined) {
        throw EpiInvalidTerm("Tuple arity is not initialized");
    }

    if (!elem || !elem->isValid()) {
        throw EpiBadArgument("Element is invalid");
    }

    mElementVector.push_back(elem);
    if (mElementVector.size() == mArity) {
        mInitialized = true;
    }
	return this;
}

ErlTerm *ErlTuple::elementAt(unsigned int index) const
        throw(EpiInvalidTerm, EpiBadArgument)
{
    if (!mArityDefined) {
        throw EpiInvalidTerm ("Tuple not initialized");
    }

    if (index > mArity) {
        throw EpiBadArgument("Index out of range [0..arity]");
    }

    if (index > mElementVector.size()) {
        throw EpiInvalidTerm("Element is not initialized");
    }

    return mElementVector[index].get();
}


bool ErlTuple::equals(const ErlTerm &t) const {
    if (!t.instanceOf(ERL_TUPLE))
        return false;

    if (!this->isValid() || !t.isValid())
        return false;

    ErlTuple *_t = (ErlTuple *) &t;

    try {
        if (mElementVector != _t->mElementVector) {
            return false;
        }
    } catch (EpiInvalidTerm &) {
        return false;
    }

    return true;
}

bool ErlTuple::internalMatch(VariableBinding* binding, ErlTerm* pattern)
        throw (EpiVariableUnbound) 
{
	Dout(dc::erlang, "Matching " << this->toString() << " with " << pattern->toString());

    if (pattern->instanceOf(ERL_VARIABLE)) {
        Dout(dc::erlang, "Pattern parameter is a variable, conmute");
        return pattern->internalMatch(binding, this);
    }

    if (!pattern->instanceOf(ERL_TUPLE))
        return false;

    if (!this->isValid() || !pattern->isValid())
        return false;

    ErlTuple *_t = (ErlTuple *) pattern;

    if (arity() != _t->arity()) {
        Dout_finish(_continue, "arity is not equal");
        return false;
    }
    try {
        erlterm_vector::iterator it, rhs, end=mElementVector.end();
        for (it=mElementVector.begin(), rhs=_t->mElementVector.begin(); it != end; ++it, ++rhs)
            if(!it->get()->internalMatch(binding, rhs->get()))
                return false;
    } catch (EpiInvalidTerm &) {
        return false;
    }

    return true;

}

std::string ErlTuple::toString(const VariableBinding *binding) const {
    if (!isValid())
        return "** INVALID TUPLE **";

    std::ostringstream oss;

    oss << "{";

    for (erlterm_vector::const_iterator it=mElementVector.begin(), end=mElementVector.end();
         it != end; ++it)
    {
        if (it != mElementVector.begin()) oss << ",";
        oss << it->get()->toString(binding);
    }
    oss << "}";

    return oss.str();

}

ErlVariable* ErlTuple::searchUnbound(const VariableBinding* binding) const {
    for (erlterm_vector::const_iterator it=mElementVector.begin(), end=mElementVector.end();
         it != end; ++it)
    {
        ErlVariable *unbound = it->get()->searchUnbound(binding);
        if (unbound)
            return unbound;
    }
    return 0;
}

ErlTerm* ErlTuple::subst(const VariableBinding* binding)
        throw (EpiInvalidTerm, EpiVariableUnbound)
{
    Dout_continue(dc::erlang, _continue, " Failed.", 
    		"["<<this<<"]"<< " ErlTerm::subst(): ");
    ErlTermPtr<ErlTuple> newTuple = new ErlTuple(arity());
    // We check if any contained term changes.
    bool change = false;
    for (erlterm_vector::iterator it=mElementVector.begin(), end=mElementVector.end();
         it != end; ++it)
    {
        ErlTerm *newElem = it->get()->subst(binding);
        // check if the pointer is different
        if (newElem != it->get()) {
            change = true;
        }
        newTuple->initElement(newElem);
    }
    // If change, return the new tuple
    if (change) {
	    Dout_finish(_continue, "Returning a new tuple with different content");
        return newTuple.drop();
    } else {
	    Dout_finish(_continue, "Returning the same tuple (no substitution)");
        return this;
    }
}

