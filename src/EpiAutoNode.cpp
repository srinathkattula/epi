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

#include "Config.hpp"

#include <set>

#ifdef USE_OPEN_THREADS
#include <OpenThreads/ScopedLock>
#elif USE_BOOST
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#endif

#include "EpiAutoNode.hpp"
#include "PlainBuffer.hpp"

using namespace epi::node;
using namespace epi::type;
using namespace epi::error;

/**
 * erase elements in a map by value
 * @return number of elements erased
 */
template<class M, class K, class T, class Compare>
int eraseByValue(M &aMap, T &value) {

    std::set<K, Compare> keys;

    for (typename M::const_iterator p = aMap.begin(); p!=aMap.end(); p++) {
        if ((*p).second == value) {
            keys.insert(K((*p).first));
        }
    }
    for (typename std::set<K, Compare>::const_iterator p = keys.begin();
         p!=keys.end(); p++)
    {
        aMap.erase(*p);
    }
    return keys.size();
}

AutoNode::AutoNode( const std::string aNodeName )
    throw( EpiBadArgument, EpiConnectionException):
        LocalNode(aNodeName), mThreadExit(false),
        #ifdef USE_BOOST
        m_threadRunning(0),
        #endif
        _connectionsMutex(), _mailboxesMutex(),
        _regmailboxesMutex(), _socketMutex(),
        mMailBoxes(), mConnections(), mRegMailBoxes(),
        mFlushConnections()
{
}

AutoNode::AutoNode( const std::string aNodeName,
                    const std::string aCookie)
        throw( EpiBadArgument, EpiConnectionException):
        LocalNode(aNodeName, aCookie), mThreadExit(false),
        #ifdef USE_BOOST
        m_threadRunning(0),
        #endif
        _connectionsMutex(), _mailboxesMutex(),
        _regmailboxesMutex(), _socketMutex(),
        mMailBoxes(), mConnections(), mRegMailBoxes(),
        mFlushConnections()
{
}

AutoNode::AutoNode( const std::string aNodeName,
                    const std::string aCookie,
                    ErlangTransport *transport)
        throw( EpiBadArgument, EpiConnectionException):
        LocalNode(aNodeName, aCookie, transport), mThreadExit(false),
        #ifdef USE_BOOST
        m_threadRunning(0),
        #endif
        _connectionsMutex(), _mailboxesMutex(), 
        _regmailboxesMutex(), _socketMutex(),
        mMailBoxes(), mConnections(), mRegMailBoxes(),
        mFlushConnections()
{
}

AutoNode::~AutoNode() {
#ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock1(_regmailboxesMutex);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock2(_connectionsMutex);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock3(_mailboxesMutex);
#elif USE_BOOST
    boost::mutex::scoped_lock lock1(_regmailboxesMutex);
    boost::mutex::scoped_lock lock2(_connectionsMutex);
    boost::mutex::scoped_lock lock3(_mailboxesMutex);
#endif
    Dout(dc::connect, "["<<this<<"]"<< "AutoNode::~AutoNode()");

    flushConnections();

    destroyConnections();
    destroyMailBoxes();

    this->close();
        
    Dout(dc::connect, "["<<this<<"]"<< "AutoNode::~AutoNode(): running = " << this->isRunning());

    if (this->isRunning()) {
        Dout(dc::connect, "["<<this<<"]"<< "AutoNode::~AutoNode(): joining thread");
        this->join();
        Dout(dc::connect, "["<<this<<"]"<< "joined");
    }
}

MailBox *AutoNode::createMailBox() {
    MailBox* mailbox = newMailBox();
    mailbox->setSender(this);
    addMailBox(mailbox);
    return mailbox;
}

void AutoNode::deattachMailBox(MailBox *mailbox) {
    mailbox->setSender(0);
    removeMailBox(mailbox);
}

void AutoNode::registerMailBox(const std::string name, MailBox *mailbox) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_regmailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_regmailboxesMutex);
    #endif
    mRegMailBoxes[name] = mailbox;
}

void AutoNode::unRegisterMailBox(const std::string name) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_regmailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_regmailboxesMutex);
    #endif
    mRegMailBoxes.erase(name);
}

void AutoNode::unRegisterMailBox(MailBox* mailbox) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_regmailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_regmailboxesMutex);
    #endif
    eraseByValue<registered_mailbox_map, std::string,
    MailBox*, std::less<std::string> >(mRegMailBoxes, mailbox);
}


void AutoNode::close() {
    mThreadExit = true;
}

void AutoNode::startAcceptor()
        throw (EpiConnectionException)
{
	// start the acceptor thread
    #ifdef USE_OPEN_THREADS
    start();
    #elif USE_BOOST
    m_thread = boost::shared_ptr<boost::thread>(
        new boost::thread(boost::bind(&AutoNode::run, this))
    );
                
    #endif
	
	// Publish the port. Try to publish it, and if it fails, try
	// to unpublish an publish it.
	try {
		this->publishPort();
	} catch (EpiConnectionException &e) {
		this->unPublishPort();
		this->publishPort();
	}

}

void AutoNode::run() {
    #ifdef USE_BOOST
    m_threadRunning = 1;
    #endif
    
    #ifdef CWDEBUG
    epi::debug::setThreadDebugMargin();
    #endif

    Dout(dc::connect, "["<<this<<"]"<< "AutoNode::run(): Thread started (" << gettid() << ")");
    
    Connection* connection;
    do {
        Dout(dc::connect, "["<<this<<"]"<< "AutoNode::run(): Waiting for connections");
        if (mThreadExit) {
            break;
        }
        try {
            // accept a connection, add and start it
            connection = this->accept(500);
		    if(connection) {             
            		//Dout(dc::connect, "["<<this<<"]"<< "AutoNode::run(): New connection from "<<
                 //   	connection->getPeer()->getNodeName());
            		addConnection(connection);
            		connection->start();
		    }
        } catch (EpiConnectionException &e) {
            // FIXME: Inform about the error
            Dout(dc::connect, "["<<this<<"]"<< "AutoNode::run(): EpiConnectionException: \"" << e.getMessage() <<"\"");
            // FIXME: Check when is necesary stop the thread
            // mThreadExit = true;
        } catch (EpiTimeout &e) {
        }
    } while(!mThreadExit);

    Dout(dc::connect, "["<<this<<"]"<< "AutoNode::run(): Thread exit (" << gettid() << ")");
    #ifdef USE_BOOST
    m_threadRunning = 0;
    m_thread.reset();
    #endif
}

/* internal info about the message formats...
 *
 * the request:
 *  -> REG_SEND {6,#Pid<bingo@aule.1.0>,'',net_kernel}
 *  {'$gen_call',{#Pid<bingo@aule.1.0>,#Ref<bingo@aule.2>},{is_auth,bingo@aule}}
 *
 * the reply:
 *  <- SEND {2,'',#Pid<bingo@aule.1.0>}
 *  {#Ref<bingo@aule.2>,yes}
 */
bool AutoNode::ping(const std::string remoteNode, long timeout) {
	if (remoteNode == this->getAliveName() ||	
		remoteNode == this->getNodeName()) {
		return true;
	}
	try {
		// Create the mailbox 
		MailBox* mailbox (this->createMailBox());
		VariableBinding binding;
		// Create the ping tuple, and the reply pattern
		ErlTerm* pingRef = createRef();
		
		ErlTermPtr<ErlTuple> pingTuple(new ErlTuple(3));
		pingTuple->initElement(new ErlAtom("$gen_call"));
		pingTuple->initElement(new ErlTuple(mailbox->self(), pingRef));
		pingTuple->initElement(new ErlTuple(new ErlAtom("is_auth"), 
					 					    new ErlAtom(getNodeName())));
		
		ErlTermPtr<ErlTuple> replyPattern(new ErlTuple(pingRef, 
										  new ErlAtom("yes")));
		
		// send it
		mailbox->send(remoteNode, "net_kernel", pingTuple.get());

		// Get the reply
		ErlTermPtr<> reply(mailbox->receive(replyPattern.get(), timeout));

		// If we get a reply, return it
		if (reply.get() != 0) {
			return true;
		}
		
	} catch (EpiException &e) {
		
	}
	return false;
		
}

void AutoNode::deliver( void *origin, EpiMessage* msg ) {
    Dout(dc::connect, "AutoNode::deliver(msg)");
    switch(msg->messageType()) {
        case ERL_MSG_ERROR:
            // FIXME: Implement a system to notify connection failure
            removeConnection((Connection *) origin);
            break;
        case ERL_MSG_SEND:
            if (1==1) { // hardcoded scope :)
                SendMessage *smsg = (SendMessage*) msg;

                Dout_continue(dc::connect, _continue, " failed.", 
                		"AutoNode::deliver(msg) -> Send type to "<<
                    	smsg->getRecipientPid()->toString());

                MailBox *recipientMailBox = getMailBox(smsg->getRecipientPid());
                if (recipientMailBox) {
                    recipientMailBox->deliver(origin, msg);
                    Dout_finish(_continue, " Sent to recipient");
                } else {
                    Dout_finish(_continue, " no recipient found, ignored");
                    // Ignore message
                    delete msg;
                }
            }
            break;
        case ERL_MSG_REG_SEND:
            if (1==1) { // hardcoded scope :)
                RegSendMessage *rsmsg = (RegSendMessage*) msg;

                Dout_continue(dc::connect, _continue, " failed.", 
                		"AutoNode::deliver(msg) -> RegSend type to "<<
                     rsmsg->getRecipientName());

                MailBox *recipientMailBox = getMailBox(rsmsg->getRecipientName());
                if (recipientMailBox) {
                    recipientMailBox->deliver(origin, msg);
                    Dout_finish(_continue, " Sent to recipient");
                } else {
                    Dout_finish(_continue, " no recipient found, ignored");
                    // Ignore message
                    delete msg;
                }
            }
            break;
        case ERL_MSG_UNLINK:
        case ERL_MSG_LINK:
        case ERL_MSG_EXIT:
        // TODO: Add code for control messages
            break;
        default:
            break;
    }

}

void AutoNode::sendBuf( epi::type::ErlPid* from,
                        epi::type::ErlPid* to,
                        epi::node::OutputBuffer* buffer )
        throw (epi::error::EpiConnectionException)
{

    if (isSameHost(to->node(), this->getNodeName(), this->getHostName())) {
        SendMessage *message = new SendMessage(to, buffer->getInputBuffer());
        deliver(this, message);
    } else {
        Connection *connection = attempConnection(to->node());

        // Encode data to output buffer for this connection
        PlainBuffer *plainbuffer = (PlainBuffer *) buffer;
        OutputBuffer *outbuffer = connection->newOutputBuffer();
        ErlTerm *t;
        do {
            t = plainbuffer->readTerm();
            if (t) {
                outbuffer->writeTerm(t);
            }
        } while (t != 0);
        connection->sendBuf(from, to, outbuffer);
    }
}

void AutoNode::sendBuf( epi::type::ErlPid* from,
                        const std::string &to,
                        epi::node::OutputBuffer* buffer )
        throw (epi::error::EpiConnectionException)
{
    RegSendMessage *message = new RegSendMessage(from, to, buffer->getInputBuffer());
    deliver(this, message);
}

void AutoNode::sendBuf( epi::type::ErlPid* from,
                        const std::string &node,
                        const std::string &to,
                        epi::node::OutputBuffer* buffer )
        throw (epi::error::EpiConnectionException)
{
    if (isSameHost(node, this->getNodeName(), this->getHostName())) {
        RegSendMessage *message = new RegSendMessage(from, to, buffer->getInputBuffer());
        deliver(this, message);
    } else {
        Connection *connection = attempConnection(node);

        // Encode data to output buffer for this connection
        PlainBuffer *plainbuffer = (PlainBuffer *) buffer;
        OutputBuffer *outbuffer = connection->newOutputBuffer();
        ErlTerm *t;
        do {
            t = plainbuffer->readTerm();
            if (t) {
                outbuffer->writeTerm(t);
            }
        } while (t != 0);

        connection->sendBuf(from, to, outbuffer);
    }
}

OutputBuffer* AutoNode::newOutputBuffer() {
    return new PlainBuffer();
}


void AutoNode::event(EpiObservable* observed, EpiEventTag event) {
}

void AutoNode::addMailBox(MailBox *mailbox) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_mailboxesMutex);
    #endif
    mMailBoxes[mailbox->self()] = mailbox;
}

MailBox *AutoNode::getMailBox(ErlPid *pid) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_mailboxesMutex);
    #endif
    if (mMailBoxes.count(pid)) {
        return mMailBoxes[pid];
    } else {
        return 0;
    }
}

MailBox *AutoNode::getMailBox(std::string name) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_regmailboxesMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_regmailboxesMutex);
    #endif
    if (mRegMailBoxes.count(name)) {
        return mRegMailBoxes[name];
    } else {
        return 0;
    }
}

void AutoNode::removeMailBox(MailBox *mailbox) {
    // I don't use ScopedLock to prevent interlock
    _mailboxesMutex.lock();
    eraseByValue<mailbox_map, ErlTermPtr<ErlPid>, MailBox*, ErlPidPtrCompare>(mMailBoxes, mailbox);
    _mailboxesMutex.unlock();
    _regmailboxesMutex.lock();
    eraseByValue<registered_mailbox_map, std::string,
    MailBox*, std::less<std::string> >(mRegMailBoxes, mailbox);
    _regmailboxesMutex.unlock();
}

Connection* AutoNode::getConnection(std::string name) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_connectionsMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_connectionsMutex);
    #endif
    if (mConnections.count(name)) {
        return mConnections[name];
    } else {
        return 0;
    }
}

void AutoNode::addConnection(Connection* connection) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_connectionsMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_connectionsMutex);
    #endif
    mConnections[connection->getPeer()->getNodeName()] = connection;
    connection->setReceiver(this);
    // Flush the connections that must be deleted
    flushConnections();
}

void AutoNode::removeConnection(Connection* connection) {
    #ifdef USE_OPEN_THREADS
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_connectionsMutex);
    #elif USE_BOOST
    boost::mutex::scoped_lock lock(_connectionsMutex);
    #endif
    int count =
            eraseByValue<connection_map, std::string, Connection*, std::less<std::string> >(mConnections, connection);
    // If count>0 => the connection belongs to this node and will be deleted
    if (count > 0) {
        mFlushConnections.push_back(connection);
    }
}

void AutoNode::flushConnections() {
    for (connection_list::const_iterator p = mFlushConnections.begin(), end=mFlushConnections.end(); p != end; ++p)
        delete *p;
    mFlushConnections.clear();
}

Connection *AutoNode::attempConnection(std::string name)
        throw (EpiConnectionException)
{
    Connection *connection = getConnection(name);
    if (connection == 0) {
        connection = this->connect(name);
        addConnection(connection);
        connection->start();
    }
    return connection;
}

void AutoNode::destroyConnections() {
    for (connection_map::const_iterator p = mConnections.begin(), end=mConnections.end(); p != end; ++p)
        delete (*p).second;
}

void AutoNode::destroyMailBoxes() {
    for (mailbox_map::const_iterator p = mMailBoxes.begin(), end=mMailBoxes.end(); p != end; ++p)
        delete (*p).second;
}
