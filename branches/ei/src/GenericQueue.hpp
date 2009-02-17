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

#ifndef __GENERICQUEUE_CPP
#define __GENERICQUEUE_CPP

#include <list>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

/**
 * Predicate to explore the queue.
 * Implement the method check that analizes the elements of the queue.
 */
class QueueGuard {
public:
    virtual ~QueueGuard() {}

    /**
     * callback with the predicate of the guard.
     * FIXME: I use void* because I don't known how do this with templates :(
     * @param elem one element of the queue.
     * @return true if the element complaints the predicate and must be
     * poped from the queue and returned from the queue. False to test next
     * element in queue
     */
    virtual bool check(void* elem) {
        return false;
    }
};


/**
 * TODO: hide implementation
 * TODO: iterators
 * Implements a generic queue for threads.
 * The queue stores pointers.
 * @param T class which pointers will be stored
*/
template <typename T> class GenericQueue {
    typedef std::list<T*> element_list;
    typedef typename std::list<T*>::iterator iterator;
public:

    /**
     * Retrieve an object from the head of the queue, or block until
     * one arrives.
     * @return  The pointer at the head of the queue.
     */
    T* get();

    /**
     * Retrieve an object from the head of the queue, blocking until
     * one arrives  or until timeout occurs.
     * @param  timeout Maximum time to block on queue, in ms. Use 0 to poll the queue.
     * @return  The object at the head of the queue, or null if none arrived in time. 
	 *  (if timeout)
     */
    T* get(long timeout);

    /**
     * Retrieve an object that complaints a predicate defined in QueueGuard class.
     * this method will block the thread until a complaint element arrives.
     * @param  guard QueueGuard with the predicate
     * @return  The object at the head of the queue, or null if none arrived in time.
     */
    T* get(QueueGuard *guard);

    /**
     * Retrieve an object that complaints a predicate defined in QueueGuard class.
     * blocking until one arrives or timeout occurs.
     * @param  guard QueueGuard with the predicate
     * @throws InterruptedException if the operation times out.
     * @return  The object at the head of the queue, or null if none arrived in time.
     */
    T* get(QueueGuard *guard, long timeout);

    /**
     * Add an object to the tail of the queue.
     * @param o Object to insert in the queue
     */
    void put(T* element);

    /**
     * Return the number of elements in the queue
     */
    int count();

    /**
     * Clear and delete all containing pointers
     */
    void flush();
private:
    // attempt to retrieve message from queue head
    T* tryGet();
    element_list mList;

	boost::mutex _queueMutex;
	boost::condition _queueCondition;
};

template <class T>
T* GenericQueue<T>::tryGet( ) {
    T* elem;
    if (!mList.empty()) {
        elem = mList.front();
        mList.pop_front();
    } else {
        elem = 0;
    }
    return elem;
}

template <class T>
T* GenericQueue<T>::get( ) {
	boost::mutex::scoped_lock lock(_queueMutex);

    T* elem;
    while ((elem = tryGet()) == 0) {
        _queueCondition.wait(_queueMutex);
    }
    return elem;
}


template <class T>
        T* GenericQueue<T>::get(long timeout)
{
	boost::mutex::scoped_lock lock(_queueMutex);
    boost::system_time currentTime = boost::get_system_time();
    boost::posix_time::milliseconds const delay(timeout);
    boost::system_time const stopTime = currentTime + delay;
    T* elem;

    while ((elem = tryGet()) == 0) {
        currentTime = boost::get_system_time();
        if (stopTime < currentTime) {
			return 0;
        }
        if (_queueCondition.timed_wait(_queueMutex, stopTime) != 0) {
			return 0;
        }
    }
    return elem;
}

template <class T>
T* GenericQueue<T>::get(QueueGuard *guard)
{
	boost::mutex::scoped_lock lock(_queueMutex);

	while (true) {
        // Iterate the list
        for (iterator i = mList.begin(); i != mList.end(); ++i) {
            T* elem = *i;
            if (guard->check(elem)) {
                mList.remove(elem);
                return elem;
            }
        }

        // No element complaints
        // Give oportunity to other
        _queueCondition.notify_one();
        _queueCondition.wait(_queueMutex);
        // Sleep
    }
}

template <class T>
T* GenericQueue<T>::get(QueueGuard *guard, long timeout)
{
	boost::mutex::scoped_lock lock(_queueMutex);
    boost::system_time currentTime = boost::get_system_time();
    boost::posix_time::milliseconds const delay(timeout);
    boost::system_time const stopTime = currentTime + delay;

    while (true) {
        // Iterate the list
        for (iterator i = mList.begin(); i != mList.end(); ++i)
        {
            T* elem = *i;
            if (guard->check(elem)) {
                mList.remove(elem);
                return elem;
            }
        }
        // No element complaints
        // Give oportunity to other
        _queueCondition.notify_one();
        currentTime = boost::get_system_time();

        if (stopTime < currentTime) {
			return 0;
        }
        if (_queueCondition.timed_wait(_queueMutex, stopTime) != 0) {
			return 0; 
        }
    }
}


template <class T> int GenericQueue<T>::count() {
	boost::mutex::scoped_lock lock(_queueMutex);
    return mList.size();
}

template <class T> void GenericQueue<T>::put(T* element)
{
	boost::mutex::scoped_lock lock(_queueMutex);

    mList.push_back(element);
    _queueCondition.notify_one();
}

template <class T> void GenericQueue<T>::flush()
{
    // Iterate the list
    for (iterator i = mList.begin(); i != mList.end(); i++)
        delete (*i);
    mList.clear();
}



#endif // __GENERICQUEUE_CPP
