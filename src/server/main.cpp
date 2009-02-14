//
// posix_main.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "server.hpp"
#include "request.hpp"

typedef ei::server::request< std::allocator<char> > RequestT;

using namespace ei::server;

#if defined(_WIN32)

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function();
    return TRUE;
  default:
    return FALSE;
  }
}

#else

#include <pthread.h>
#include <signal.h>

sigset_t block_all_signals()
{
    // Block all signals for background thread.
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
    return old_mask;
}

void restore_sig_and_wait(sigset_t& old_mask)
{
    // Restore previous signals.
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

    // Wait for signal indicating time to shut down.
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGINT);
    sigaddset(&wait_mask, SIGQUIT);
    sigaddset(&wait_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
    int sig = 0;
    sigwait(&wait_mask, &sig);
}

#endif // !defined(_WIN32)

int main(int argc, char* argv[])
{
    bool use_pipe = false;
    std::string addr("0.0.0.0");
    std::string port;
    
    try
    {
        // Check command line arguments.
        for (int i=1; i < argc; i++) {
            if (strcmp(argv[i], "-pipe") == 0)
                use_pipe = true;
            else if (strcmp(argv[i], "-a") == 0 && i+1 < argc && argv[i+1][0] != '-')
                addr = argv[++i];
            else if (strcmp(argv[i], "-p") == 0 && i+1 < argc && argv[i+1][0] != '-')
                port = argv[++i];
        }
                
        if (!use_pipe && port.empty())
        {
            std::string s(argv[0]);
            size_t n = s.find_last_of('/');
            if (n != std::string::npos)
                s.erase(0, n+1);
            std::cerr << "Usage: " << s << " [-pipe] [-a <address>] [-p <port>]\n"
                      << "  Either -pipe or -p option is required" << std::endl;
            return 1;
        }

#if defined(_WIN32)

        // Initialise server.
        if (use_pipe) {
            pipe_server<RequestT> s(3, 4);

            // Set console control handler to allow server to be stopped.
            console_ctrl_function = boost::bind(&pipe_server<RequestT>::stop, &s);
            SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

            // Run the server until stopped.
            s.run();
        } else {
            tcp_server<RequestT> s(addr, port);

            // Set console control handler to allow server to be stopped.
            console_ctrl_function = boost::bind(&tcp_server<RequestT>::stop, &s);
            SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

            // Run the server until stopped.
            s.run();
        }
#else

        sigset_t old_mask = block_all_signals();
        
        if (use_pipe) {
            // Run server in background thread.
            pipe_server<RequestT> s(3, 4);
            boost::thread t(boost::bind(&pipe_server<RequestT>::run, &s));

            restore_sig_and_wait(old_mask);
            
            // Stop the server.
            s.stop();
            t.join();
        } else {
            // Run server in background thread.
            tcp_server<RequestT> s(addr, port);
            boost::thread t(boost::bind(&tcp_server<RequestT>::run, &s));

            restore_sig_and_wait(old_mask);
            
            // Stop the server.
            s.stop();
            t.join();
        }
#endif
        std::cout << "Server stopped" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}
