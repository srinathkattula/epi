#include "Config.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "epi.hpp"


// Use namespaces
using namespace epi::type;
using namespace epi::error;
using namespace epi::node;

#ifdef CWDEBUG
pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


int main (int argc, char **args) {
	
	if (argc < 3) {
		std::cout << "Use: " << args[0] << 
			" <local node name> <remote node name> [<cookie>]" <<
			std::endl;
		exit(0);
	}

    #ifdef CWDEBUG
    std::ios::sync_with_stdio(false);
    Debug( libcw_do.set_ostream(&std::cout, &cout_mutex) );

    Debug( libcw_do.on() );
    ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on());
    Debug( dc::bfd.off() );
    Debug( list_channels_on(libcw_do) );
    
    /*
    Debug( dc::notice.on() );
    Debug( dc::erlang.on() );
    Debug( dc::erlang_warning.on() );
    Debug( dc::erlang_memory.on() );
    */

    epi::debug::setThreadDebugMargin();

    Dout( dc::notice, "Entering main thread " << gettid() );
    #endif


    const std::string LOCALNODE(args[1]);
    const std::string REMOTENODE(args[2]);
    const char* COOKIE = argc >= 4 ? args[3] : "";

    int result = 0;

    try {
        // Create the node
        AutoNode node(LOCALNODE, COOKIE);

        // Get a mailbox. The node has the pointer owership!!!
        MailBox *mailbox = node.createMailBox();

		// Create the tuple {self(), hello}
        ErlTermPtr<ErlTuple> tuple(new ErlTuple(2));
		tuple->initElement(mailbox->self())->initElement(new ErlAtom("hello"));

        for(int i=0; i < 5; i++) {
            // Send the term to a server in the remote node
            mailbox->send(REMOTENODE, "reply_server", tuple.get());

            // Receive the response
            ErlTermPtr<> received(mailbox->receive());

            // Print it
            std::cout << "Received response: " << received->toString() << std::endl;
            sleep(5);
        }
    } catch (EpiException &e) {
        std::cout << "Exception caught: " << e.getMessage() << std::endl;
        result = 1;

    }

    Dout( dc::notice, "Leaving main thread " << gettid() << " (result=" << result << ")" );
    
    return result;
}
