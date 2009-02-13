#ifndef _SERVER_DEBUG_HPP_
#define _SERVER_DEBUG_HPP_

#ifdef SRV_DEBUG
#define DBG( x ) std::cout << x << std::endl
#else
#define DBG( x )
#endif

#endif
